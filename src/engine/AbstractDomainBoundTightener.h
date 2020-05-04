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

        // Step 2: propagate through the hidden layers
        for ( _currentLayer = 1; _currentLayer < _numberOfLayers; ++_currentLayer )
        {
            // Apply the weighted sum
            printf( "Starting affine transformation for layer %u\n", _currentLayer );
            performAffineTransformation();

            printf( "Currently known bounds:\n" );
            for ( unsigned i = 0; i < (*_layerSizes)[1]; ++i )
            {
                printf( "ws_1_%u: [%lf, %lf]\n", i, _lowerBoundsWeightedSums[1][i],
                        _upperBoundsWeightedSums[1][i] );
            }

             // exit( 1 );

            // Apply the activation function, except for the output layer
            if ( _currentLayer != _numberOfLayers - 1 )
                applyActivationFunction();

            exit( 1 );
        }

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

    String weightedSumVariableToString( NeuronIndex index )
    {
        return Stringf( "ws_%u_%u", index._layer, index._neuron );
    }

    String activationResultVariableToString( NeuronIndex index )
    {
        return Stringf( "ar_%u_%u", index._layer, index._neuron );
    }

    void performAffineTransformation()
    {
        /*
          We create constraints that include:

            - The bounds for the previous layer
            - The variables of the current layer as a function of the
              activation results from the previous layer
        */

        unsigned previousLayerSize = (*_layerSizes)[_currentLayer - 1];
        unsigned currentLayerSize = (*_layerSizes)[_currentLayer];

        ap_lincons1_array_t constraintArray = ap_lincons1_array_make( _apronEnvironment,
                                                                      ( 2 * previousLayerSize ) + currentLayerSize );

        // Bounds for the previous layer
        for ( unsigned i = 0; i < previousLayerSize; ++i )
        {
            double lb = _lowerBoundsActivations[_currentLayer - 1][i];
            double ub = _upperBoundsActivations[_currentLayer - 1][i];

            // ws - lb >= 0
            ap_linexpr1_t exprLb = ap_linexpr1_make( _apronEnvironment,
                                                     AP_LINEXPR_SPARSE,
                                                     1 );
            ap_lincons1_t consLb = ap_lincons1_make( AP_CONS_SUPEQ,
                                                     &exprLb,
                                                     NULL );

            ap_lincons1_set_list( &consLb,
                                  AP_COEFF_S_INT, 1, activationResultVariableToString( NeuronIndex( _currentLayer - 1, i ) ).ascii(),
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
                                  AP_COEFF_S_INT, -1, activationResultVariableToString( NeuronIndex( _currentLayer - 1, i ) ).ascii(),
                                  AP_CST_S_DOUBLE, ub,
                                  AP_END );

            ap_lincons1_array_set( &constraintArray, ( i * 2 ) + 1, &consUb );
        }

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
                                  AP_COEFF_S_INT, -1, weightedSumVariableToString( NeuronIndex( _currentLayer, i ) ).ascii(),
                                  AP_CST_S_DOUBLE, (*_bias)[NeuronIndex( _currentLayer, i )],
                                  AP_END );

            for ( unsigned j = 0; j < previousLayerSize; ++j )
            {
                double weight = _weights[_currentLayer - 1][j * currentLayerSize + i];
                ap_lincons1_set_list( &cons,
                                      AP_COEFF_S_DOUBLE, weight, activationResultVariableToString( NeuronIndex( _currentLayer - 1, j ) ).ascii(),
                                      AP_END );
            }

            // Register the constraint
            ap_lincons1_array_set( &constraintArray, previousLayerSize * 2 + i, &cons );
        }

        _wsAbstractValue = ap_abstract1_of_lincons_array( _apronManager,
                                                          _apronEnvironment,
                                                          &constraintArray );

        fprintf(stdout,"WS av:\n");
        ap_abstract1_fprint(stdout,_apronManager,&_wsAbstractValue);

        ap_lincons1_array_clear( &constraintArray );
    }

    void applyActivationFunction()
    {
        unsigned currentLayerSize = (*_layerSizes)[_currentLayer];

        // For now, we assume that all activation functions are ReLUs
        DEBUG({
                for ( unsigned i = 0; i < currentLayerSize; ++i )
                {
                    NeuronIndex index( _currentLayer, i );
                    ASSERT( (*_neuronToActivationFunction).exists( index ) );
                    ASSERT( (*_neuronToActivationFunction).at( index ) == PiecewiseLinearFunctionType::RELU );
                }
            });

        // Process one neuron at a time
        for ( unsigned currentNeuron = 0; currentNeuron < currentLayerSize; ++currentNeuron )
        {
            ap_lincons1_array_t activeConstraintArray = ap_lincons1_array_make( _apronEnvironment, 1 );

            ap_linexpr1_t activeExpr = ap_linexpr1_make( _apronEnvironment, AP_LINEXPR_SPARSE, 1 );
            ap_lincons1_t activeCons = ap_lincons1_make( AP_CONS_SUPEQ, &activeExpr, NULL );

            // Weighted sum >= 0
            ap_lincons1_set_list( &activeCons,
                                  AP_COEFF_S_INT,1, weightedSumVariableToString( NeuronIndex( _currentLayer, currentNeuron ) ).ascii(),
                                  AP_END);

            ap_lincons1_array_set( &activeConstraintArray, 0, &activeCons );

            ap_abstract1_t activeConstraintAbstractValue = ap_abstract1_of_lincons_array( _apronManager, _apronEnvironment, &activeConstraintArray );

            fprintf(stdout,"active constraint:\n");
            ap_abstract1_fprint(stdout,_apronManager,&activeConstraintAbstractValue);

            // Meet the weighted sum abstract value with the active cosntraint

            bool notDestructive = false;
            ap_abstract1_t meet = ap_abstract1_meet( _apronManager,
                                                     notDestructive,
                                                     &_wsAbstractValue,
                                                     &activeConstraintAbstractValue );

            fprintf(stdout,"result of meet:\n");
            ap_abstract1_fprint(stdout,_apronManager,&meet);

            exit( 1 );
            // For the active case, the active case is the identity
            // function, so no addition processing is required.
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
        printf( "Allocate starting...\n" );
        _apronManager = box_manager_alloc();
        printf( "\t\tDone!\n" );

        // Count the total number of variables
        _totalNumberOfVariables = 0;
        for ( unsigned i = 0; i < _numberOfLayers; ++i )
        {
            _totalNumberOfVariables += (*_layerSizes)[i];
            if ( ( i != 0 ) && ( i != _numberOfLayers - 1 ) )
                _totalNumberOfVariables += (*_layerSizes)[i];
        }

        // Allocate the array
        _variableNames = new char *[_totalNumberOfVariables];

        // Populate the array
        unsigned counter = 0;
        for ( unsigned i = 0; i < _numberOfLayers; ++i )
        {
            for ( unsigned j = 0; j < (*_layerSizes)[i]; ++j )
            {
                NeuronIndex index( i, j );

                if ( i != 0 )
                {
                    String wsVarName = weightedSumVariableToString( index );
                    _variableNames[counter] = new char[wsVarName.length() + 1];
                    strncpy( _variableNames[counter], wsVarName.ascii(), wsVarName.length() );
                    _variableNames[counter][wsVarName.length()] = '\0';
                    ++counter;
                }

                if ( i != _numberOfLayers - 1 )
                {
                    String arVarName = activationResultVariableToString( index );
                    _variableNames[counter] = new char[arVarName.length() + 1];
                    strncpy( _variableNames[counter], arVarName.ascii(), arVarName.length() );
                    _variableNames[counter][arVarName.length()] = '\0';
                    ++counter;
                }
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

        printf( "Deallocate starting...\n" );
        ap_manager_free( _apronManager );
        printf( "\t\tDone!\n" );
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
