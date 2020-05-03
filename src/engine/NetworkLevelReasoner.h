/*********************                                                        */
/*! \file NetworkLevelReasoner.h
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

#ifndef __NetworkLevelReasoner_h__
#define __NetworkLevelReasoner_h__

#include "ITableau.h"
#include "Map.h"
#include "PiecewiseLinearFunctionType.h"
#include "Tightening.h"
#include "NeuronIndex.h"

#include "AbstractDomainBoundTightener.h"

#include "ap_global0.h"
#include "ap_global1.h"

#include "box.h"
#include "oct.h"
#include "pk.h"
#include "pkeq.h"

/*
  A class for performing operations that require knowledge of network
  level structure and topology.
*/

class NetworkLevelReasoner
{
public:
    NetworkLevelReasoner();
    ~NetworkLevelReasoner();

    static bool functionTypeSupported( PiecewiseLinearFunctionType type );

    void setNumberOfLayers( unsigned numberOfLayers );
    void setLayerSize( unsigned layer, unsigned size );
    void setNeuronActivationFunction( unsigned layer, unsigned neuron, PiecewiseLinearFunctionType activationFuction );
    void setWeight( unsigned sourceLayer, unsigned sourceNeuron, unsigned targetNeuron, double weight );
    void setBias( unsigned layer, unsigned neuron, double bias );

    /*
      A method that allocates all internal memory structures, based on
      the network's topology. Should be invoked after the layer sizes
      have been provided.
    */
    void allocateMemoryByTopology();

    /*
      Mapping from node indices to the variables representing their
      weighted sum values and activation result values.
    */
    void setWeightedSumVariable( unsigned layer, unsigned neuron, unsigned variable );
    unsigned getWeightedSumVariable( unsigned layer, unsigned neuron ) const;
    void setActivationResultVariable( unsigned layer, unsigned neuron, unsigned variable );
    unsigned getActivationResultVariable( unsigned layer, unsigned neuron ) const;
    const Map<NeuronIndex, unsigned> &getIndexToWeightedSumVariable();
    const Map<NeuronIndex, unsigned> &getIndexToActivationResultVariable();

    /*
      Mapping from node indices to the nodes' assignments, as computed
      by evaluate()
    */
    const Map<NeuronIndex, double> &getIndexToWeightedSumAssignment();
    const Map<NeuronIndex, double> &getIndexToActivationResultAssignment();

    /*
      Interface methods for performing operations on the network.
    */
    void evaluate( double *input, double *output );

    /*
      Duplicate the reasoner
    */
    void storeIntoOther( NetworkLevelReasoner &other ) const;

    /*
      Methods that are typically invoked by the preprocessor, to
      inform us of changes in variable indices or if a variable has
      been eliminated
    */
    void eliminateVariable( unsigned variable, double value );
    void updateVariableIndices( const Map<unsigned, unsigned> &oldIndexToNewIndex,
                                const Map<unsigned, unsigned> &mergedVariables );

    /*
      Bound propagation methods:

        - obtainCurrentBounds: obtain the current bounds on all variables
          from the tableau.

        - Interval arithmetic: compute the bounds of a layer's neurons
          based on the concrete bounds of the previous layer.

        - Symbolic: for each neuron in the network, we compute lower
          and upper bounds on the lower and upper bounds of the
          neuron. This bounds are expressed as linear combinations of
          the input neurons. Sometimes these bounds let us simplify
          expressions and obtain tighter bounds (e.g., if the upper
          bound on the upper bound of a ReLU node is negative, that
          ReLU is inactive and its output can be set to 0.

          Initialize should be called once, before the bound
          propagation is performed.
    */

    void setTableau( const ITableau *tableau );
    void obtainCurrentBounds();
    void intervalArithmeticBoundPropagation();
    void symbolicBoundPropagation();

    void getConstraintTightenings( List<Tightening> &tightenings ) const;

    /*
      For debugging purposes: dump the network topology
    */
    void dumpTopology() const;

private:
    unsigned _numberOfLayers;
    Map<unsigned, unsigned> _layerSizes;
    Map<NeuronIndex, PiecewiseLinearFunctionType> _neuronToActivationFunction;
    double **_weights;
    double **_positiveWeights;
    double **_negativeWeights;
    Map<NeuronIndex, double> _bias;

    unsigned _maxLayerSize;
    unsigned _inputLayerSize;

    double *_work1;
    double *_work2;

    const ITableau *_tableau;

    void freeMemoryIfNeeded();

    /*
      Mappings of indices to weighted sum and activation result variables
    */
    Map<NeuronIndex, unsigned> _indexToWeightedSumVariable;
    Map<NeuronIndex, unsigned> _indexToActivationResultVariable;
    Map<unsigned, NeuronIndex> _weightedSumVariableToIndex;
    Map<unsigned, NeuronIndex> _activationResultVariableToIndex;

    /*
      Store the assignment to all variables when evaluate() is called
    */
    Map<NeuronIndex, double> _indexToWeightedSumAssignment;
    Map<NeuronIndex, double> _indexToActivationResultAssignment;

    /*
      Store eliminated variables
    */
    Map<NeuronIndex, double> _eliminatedWeightedSumVariables;
    Map<NeuronIndex, double> _eliminatedActivationResultVariables;

    /*
      Work space for bound tightening
    */
    double **_lowerBoundsWeightedSums;
    double **_upperBoundsWeightedSums;
    double **_lowerBoundsActivations;
    double **_upperBoundsActivations;

    /*
      Work space for symbolic bound propagation
    */
    double *_currentLayerLowerBounds;
    double *_currentLayerUpperBounds;
    double *_currentLayerLowerBias;
    double *_currentLayerUpperBias;

    double *_previousLayerLowerBounds;
    double *_previousLayerUpperBounds;
    double *_previousLayerLowerBias;
    double *_previousLayerUpperBias;

    /*
      Helper functions that perform symbolic bound propagation for a
      single neuron, according to its activation function
    */
    void reluSymbolicPropagation( const NeuronIndex &index, double &lbLb, double &lbUb, double &ubLb, double &ubUb );
    void absoluteValueSymbolicPropagation( const NeuronIndex &index, double &lbLb, double &lbUb, double &ubLb, double &ubUb );

    static void log( const String &message );

public:
    void dummy()
    {
        const char *var_x = "x";
        const char *var_y = "y";
        const char *var_z = "z";
        const char *var_u = "u";
        const char *var_w = "w";
        const char *var_v = "v";

        ap_manager_t *man = box_manager_alloc();




        const char *vars[6] = { var_x, var_y, var_z, var_u, var_w, var_v };

        // [6] = {
        //     "x","y","z","u","w","v"
        // };
        ap_environment_t* env = ap_environment_alloc((void **)&vars[0],3,(void **)&vars[3],3);

        /* =================================================================== */
        /* Creation of polyhedra
           1/2x+2/3y=1, [1,2]<=z+2w<=4 */
        /* =================================================================== */

        /* 0. Create the array */
        ap_lincons1_array_t array = ap_lincons1_array_make(env,3);

        /* 1.a Creation of an equality constraint 1/2x+2/3y=1 */
        ap_linexpr1_t expr = ap_linexpr1_make(env,AP_LINEXPR_SPARSE,2);
        ap_lincons1_t cons = ap_lincons1_make(AP_CONS_EQ,&expr,NULL);
        /* Now expr is memory-managed by cons */

        /* 1.b Fill the constraint */
        ap_lincons1_set_list(&cons,
                             AP_COEFF_S_FRAC,1,2,"x",
                             AP_COEFF_S_FRAC,2,3,"y",
                             AP_CST_S_INT,1,
                             AP_END);
        /* 1.c Put in the array */
        ap_lincons1_array_set(&array,0,&cons);
        /* Now cons is memory-managed by array */

        /* 2.a Creation of an inequality constraint [1,2]<=z+2w */
        expr = ap_linexpr1_make(env,AP_LINEXPR_SPARSE,2);
        cons = ap_lincons1_make(AP_CONS_SUPEQ,&expr,NULL);
        /* The old cons is not lost, because it is stored in the array.
           It would be an error to clear it (same for expr). */
        /* 2.b Fill the constraint */
        ap_lincons1_set_list(&cons,
                             AP_COEFF_S_INT,1,"z",
                             AP_COEFF_S_DOUBLE,2.0,"w",
                             AP_CST_I_INT,-2,-1,
                             AP_END);
        /* 2.c Put in the array */
        ap_lincons1_array_set(&array,1,&cons);

        /* 2.a Creation of an inequality constraint */
        expr = ap_linexpr1_make(env,AP_LINEXPR_SPARSE,2);
        cons = ap_lincons1_make(AP_CONS_SUPEQ,&expr,NULL);
        /* The old cons is not lost, because it is stored in the array.
           It would be an error to clear it (same for expr). */
        /* 2.b Fill the constraint */
        ap_lincons1_set_list(&cons,
                             AP_COEFF_S_INT,1,"z",
                             AP_COEFF_S_DOUBLE,2.0,"w",
                             AP_CST_I_INT,-2,-1,
                             AP_END);
        /* 2.c Put in the array */
        ap_lincons1_array_set(&array,1,&cons);

        /* 3.a Creation of an inequality constraint by duplication and
           modification z+2w<=4 */
        cons = ap_lincons1_copy(&cons);
        /* 3.b Fill the constraint (by negating the existing coefficients) */
        expr = ap_lincons1_linexpr1ref(&cons);
        {
            ap_coeff_t* pcoeff;
            ap_var_t var;
            size_t i;
            ap_linexpr1_ForeachLinterm1(&expr,i,var,pcoeff){
                ap_coeff_neg(pcoeff,pcoeff);
            }
        }
        ap_linexpr1_set_cst_scalar_int(&expr,4);
        /* 3.c Put in the array */
        ap_lincons1_array_set(&array,2,&cons);

        /* 4. Creation of an abstract value */
        ap_abstract1_t abs = ap_abstract1_of_lincons_array(man,env,&array);

        fprintf(stdout,"Abstract value:\n");
        ap_abstract1_fprint(stdout,man,&abs);

        /* deallocation */
        ap_lincons1_array_clear(&array);

        ap_manager_free(man);
    }
};

#endif // __NetworkLevelReasoner_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
