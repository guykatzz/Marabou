/*********************                                                        */
/*! \file AbstractDomainBoundTightener.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __AbstractDomainBoundTightener_h__
#define __AbstractDomainBoundTightener_h__

#include "Debug.h"
#include "MStringf.h"
#include "NeuronIndex.h"
#include "PiecewiseLinearFunctionType.h"


#include "ap_global0.h"
#include "ap_global1.h"

#include "box.h"

#include "oct.h"
#include "pk.h"
#include "pkeq.h"

/*
  A superclass for performing abstract-interpretation-based bound
  tightening. This is a virtual class: a child class must provide the
  specific of the actual abstract domain that is being used (e.g.,
  box, polyhedron).
*/

class AbstractDomainBoundTightener
{
public:

    void initialize( unsigned numberOfLayers,
                     const Map<unsigned, unsigned> *layerSizes,
                     const Map<NeuronIndex, PiecewiseLinearFunctionType> *neuronToActivationFunction,
                     const double **weights,
                     const Map<NeuronIndex, double> *bias,
                     double **lowerBoundsWeightedSums,
                     double **upperBoundsWeightedSums,
                     double **lowerBoundsActivations,
                     double **upperBoundsActivations
                     )
    {
        _numberOfLayers = numberOfLayers;
        _layerSizes = layerSizes;
        _neuronToActivationFunction = neuronToActivationFunction;
        _weights = weights;
        _bias = bias;

        _lowerBoundsWeightedSums = lowerBoundsWeightedSums;
        _upperBoundsWeightedSums = upperBoundsWeightedSums;
        _lowerBoundsActivations = lowerBoundsActivations;
        _upperBoundsActivations = upperBoundsActivations;
    }

    void run()
    {
        allocate();

        // Step 1: construct the input abstract value
        constructInputAbstractValue();

        // Step 2: propagate through the hidden layers
        for ( _currentLayer = 1; _currentLayer < _numberOfLayers - 1; ++_currentLayer )
        {
            performAffineTransformation();
            performActivation();
            exit( 1 );
        }

        // TODO: handle output layer

        deallocate();
    }

private:
    unsigned _numberOfLayers;
    const Map<unsigned, unsigned> *_layerSizes;
    const Map<NeuronIndex, PiecewiseLinearFunctionType> *_neuronToActivationFunction;
    const double **_weights;
    const Map<NeuronIndex, double> *_bias;

    double **_lowerBoundsWeightedSums;
    double **_upperBoundsWeightedSums;
    double **_lowerBoundsActivations;
    double **_upperBoundsActivations;

    unsigned _currentLayer;

    ap_abstract1_t _currentAV;

    String variableToString( NeuronIndex index )
    {
        return Stringf( "x_%u_%u", index._layer, index._neuron );
    }

    String weightedSumVariableToString( NeuronIndex index )
    {
        return Stringf( "ws_%u_%u", index._layer, index._neuron );
    }

    String activationResultVariableToString( NeuronIndex index )
    {
        return Stringf( "ar_%u_%u", index._layer, index._neuron );
    }

    void constructInputAbstractValue()
    {
        unsigned inputLayerSize = (*_layerSizes)[0];

        ap_lincons1_array_t constraintArray = ap_lincons1_array_make( _apronEnvironment, 2 * inputLayerSize );

        // Populate the constraints: lower and upper bounds
        for ( unsigned i = 0; i < inputLayerSize; ++i )
        {
            double lb = _lowerBoundsActivations[0][i];
            double ub = _upperBoundsActivations[0][i];

            // x - lb >= 0
            ap_linexpr1_t exprLb = ap_linexpr1_make( _apronEnvironment,
                                                     AP_LINEXPR_SPARSE,
                                                     1 );
            ap_lincons1_t consLb = ap_lincons1_make( AP_CONS_SUPEQ,
                                                     &exprLb,
                                                     NULL );

            ap_lincons1_set_list( &consLb,
                                  AP_COEFF_S_INT, 1, variableToString( NeuronIndex( 0, i ) ).ascii(),
                                  AP_CST_S_DOUBLE, -lb,
                                  AP_END );

            ap_lincons1_array_set( &constraintArray, i * 2, &consLb );


            // - ws + ub >= 0
            ap_linexpr1_t exprUb = ap_linexpr1_make( _apronEnvironment,
                                                     AP_LINEXPR_SPARSE,
                                                     1 );
            ap_lincons1_t consUb = ap_lincons1_make( AP_CONS_SUPEQ,
                                                     &exprUb,
                                                     NULL );

            ap_lincons1_set_list( &consUb,
                                  AP_COEFF_S_INT, -1, variableToString( NeuronIndex( 0, i ) ).ascii(),
                                  AP_CST_S_DOUBLE, ub,
                                  AP_END );

            ap_lincons1_array_set( &constraintArray, ( i * 2 ) + 1, &consUb );
        }

        // Extract the abstract value
        _currentAV = ap_abstract1_of_lincons_array( _apronManager,
                                                    _apronEnvironment,
                                                    &constraintArray );

        printf( "_currentAV after input layer construction:\n");
        ap_abstract1_fprint( stdout, _apronManager, &_currentAV );
        ap_lincons1_array_clear( &constraintArray );
    }

    void performAffineTransformation()
    {
        unsigned previousLayerSize = (*_layerSizes)[_currentLayer - 1];
        unsigned currentLayerSize = (*_layerSizes)[_currentLayer];

        ap_lincons1_array_t constraintArray = ap_lincons1_array_make( _apronEnvironment, currentLayerSize );

        // Weight equations
        for ( unsigned i = 0; i < currentLayerSize; ++i )
        {
            ap_linexpr1_t expr = ap_linexpr1_make( _apronEnvironment,
                                                   AP_LINEXPR_SPARSE,
                                                   previousLayerSize + 1 );
            ap_lincons1_t cons = ap_lincons1_make( AP_CONS_EQ,
                                                   &expr,
                                                   NULL );

            // Add the target weighted sum variable and the bias
            ap_lincons1_set_list( &cons,
                                  AP_COEFF_S_INT, -1, variableToString( NeuronIndex( _currentLayer, i ) ).ascii(),
                                  AP_CST_S_DOUBLE, (*_bias)[NeuronIndex( _currentLayer, i )],
                                  AP_END );

            for ( unsigned j = 0; j < previousLayerSize; ++j )
            {
                double weight = _weights[_currentLayer - 1][j * currentLayerSize + i];
                ap_lincons1_set_list( &cons,
                                      AP_COEFF_S_DOUBLE, weight, variableToString( NeuronIndex( _currentLayer - 1, j ) ).ascii(),
                                      AP_END );
            }

            // Register the constraint
            ap_lincons1_array_set( &constraintArray, i, &cons );
        }

        _currentAV = ap_abstract1_of_lincons_array( _apronManager,
                                                    _apronEnvironment,
                                                    &constraintArray );

        printf( "_currentAV after affine transformation:\n");
        ap_abstract1_fprint( stdout, _apronManager, &_currentAV );
        ap_lincons1_array_clear( &constraintArray );
    }

    void performActivation()
    {
        unsigned currentLayerSize = (*_layerSizes)[_currentLayer];

        for ( unsigned currentNeuron = 0; currentNeuron < currentLayerSize; ++currentNeuron )
        {
            /*
              Active case: neuron is positive and unchanged
            */

            ap_lincons1_array_t activeConstraintArray = ap_lincons1_array_make( _apronEnvironment, 1 );

            // Weight equations
            ap_linexpr1_t activeExpr = ap_linexpr1_make( _apronEnvironment,
                                                         AP_LINEXPR_SPARSE,
                                                         currentNeuron );
            ap_lincons1_t activeCons = ap_lincons1_make( AP_CONS_SUPEQ,
                                                         &activeExpr,
                                                         NULL );

            // Neuron is positive
            ap_lincons1_set_list( &activeCons,
                                  AP_COEFF_S_INT, 1, variableToString( NeuronIndex( _currentLayer, currentNeuron ) ).ascii(),
                                  AP_END );

            ap_lincons1_array_set( &activeConstraintArray, 0, &activeCons );

            ap_abstract1_t activeAV = ap_abstract1_of_lincons_array( _apronManager,
                                                                     _apronEnvironment,
                                                                     &activeConstraintArray );

            /*
              Inactive case: neuron is zero
            */

            ap_lincons1_array_t inactiveConstraintArray = ap_lincons1_array_make( _apronEnvironment, 1 );

            // Weight equations
            ap_linexpr1_t inactiveExpr = ap_linexpr1_make( _apronEnvironment,
                                                           AP_LINEXPR_SPARSE,
                                                           currentNeuron );
            ap_lincons1_t inactiveCons = ap_lincons1_make( AP_CONS_EQ,
                                                           &inactiveExpr,
                                                           NULL );

            // Neuron is positive
            ap_lincons1_set_list( &inactiveCons,
                                  AP_COEFF_S_INT, 1, variableToString( NeuronIndex( _currentLayer, currentNeuron ) ).ascii(),
                                  AP_END );

            ap_lincons1_array_set( &inactiveConstraintArray, 0, &inactiveCons );

            ap_abstract1_t inactiveAV = ap_abstract1_of_lincons_array( _apronManager,
                                                                       _apronEnvironment,
                                                                       &inactiveConstraintArray );

            /*
              Putting it all together: two meets and a join
            */
            bool notDestructive = false;

            ap_abstract1_t meetActive = ap_abstract1_meet( _apronManager,
                                                           notDestructive,
                                                           &_currentAV,
                                                           &activeAV );

            ap_abstract1_t meetInactive = ap_abstract1_meet( _apronManager,
                                                             notDestructive,
                                                             &_currentAV,
                                                             &inactiveAV );

            _currentAV = ap_abstract1_join( _apronManager,
                                            notDestructive,
                                            &meetActive,
                                            &meetInactive );

            exit( 0 );
        }
    }

    ap_abstract1_t _wsAbstractValue;
    ap_manager_t *_apronManager;
    char *_apronVariables;

    char **_variableNames;
    unsigned _totalNumberOfVariables;

    ap_environment_t *_apronEnvironment;

    void allocate()
    {
        _apronManager = box_manager_alloc();

        // Count the total number of variables
        _totalNumberOfVariables = 0;
        for ( unsigned i = 0; i < _numberOfLayers; ++i )
            _totalNumberOfVariables += (*_layerSizes)[i];

        // Allocate the array
        _variableNames = new char *[_totalNumberOfVariables];

        // Populate the array
        unsigned counter = 0;
        for ( unsigned i = 0; i < _numberOfLayers; ++i )
        {
            for ( unsigned j = 0; j < (*_layerSizes)[i]; ++j )
            {
                NeuronIndex index( i, j );

                String varName = variableToString( index );
                _variableNames[counter] = new char[varName.length() + 1];
                strncpy( _variableNames[counter], varName.ascii(), varName.length() );
                _variableNames[counter][varName.length()] = '\0';
                ++counter;
            }
        }

        _apronEnvironment = ap_environment_alloc( NULL,
                                                  0,
                                                  (void **)&_variableNames[0],
                                                  _totalNumberOfVariables );
    }

    void deallocate()
    {
        ap_environment_free( _apronEnvironment );

        for ( unsigned i = 0; i < _totalNumberOfVariables; ++i )
            delete[] _variableNames[i];
        delete[] _variableNames;

        ap_manager_free( _apronManager );
    }
};

#endif // __AbstractDomainBoundTightener_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
