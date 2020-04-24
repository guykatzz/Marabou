/*********************                                                        */
/*! \file Test_NetworkLevelReasoner.h
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

#include <cxxtest/TestSuite.h>

#include "FloatUtils.h"
#include "MockTableau.h"
#include "NetworkLevelReasoner.h"
#include "Tightening.h"

class MockForNetworkLevelReasoner
{
public:
};

class NetworkLevelReasonerTestSuite : public CxxTest::TestSuite
{
public:
    MockForNetworkLevelReasoner *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForNetworkLevelReasoner );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void populateNetwork( NetworkLevelReasoner &nlr )
    {
        /*
                a
          x           d    f
                b
          y           e    g
                c
        */

        nlr.setNumberOfLayers( 4 );

        nlr.setLayerSize( 0, 2 );
        nlr.setLayerSize( 1, 3 );
        nlr.setLayerSize( 2, 2 );
        nlr.setLayerSize( 3, 2 );

        nlr.allocateMemoryByTopology();

        // Weights
        nlr.setWeight( 0, 0, 0, 1 );
        nlr.setWeight( 0, 0, 1, 2 );
        nlr.setWeight( 0, 1, 1, -3 );
        nlr.setWeight( 0, 1, 2, 1 );

        nlr.setWeight( 1, 0, 0, 1 );
        nlr.setWeight( 1, 0, 1, -1 );
        nlr.setWeight( 1, 1, 0, 1 );
        nlr.setWeight( 1, 1, 1, 1 );
        nlr.setWeight( 1, 2, 0, -1 );
        nlr.setWeight( 1, 2, 1, -1 );

        nlr.setWeight( 2, 0, 0, 1 );
        nlr.setWeight( 2, 0, 1, 1 );
        nlr.setWeight( 2, 1, 1, 3 );

        // Biases
        nlr.setBias( 1, 0, 1 );
        nlr.setBias( 2, 1, 2 );

        // Variable indexing
        nlr.setActivationResultVariable( 0, 0, 0 );
        nlr.setActivationResultVariable( 0, 1, 1 );

        nlr.setWeightedSumVariable( 1, 0, 2 );
        nlr.setActivationResultVariable( 1, 0, 3 );
        nlr.setWeightedSumVariable( 1, 1, 4 );
        nlr.setActivationResultVariable( 1, 1, 5 );
        nlr.setWeightedSumVariable( 1, 2, 6 );
        nlr.setActivationResultVariable( 1, 2, 7 );

        nlr.setWeightedSumVariable( 2, 0, 8 );
        nlr.setActivationResultVariable( 2, 0, 9 );
        nlr.setWeightedSumVariable( 2, 1, 10 );
        nlr.setActivationResultVariable( 2, 1, 11 );

        nlr.setWeightedSumVariable( 3, 0, 12 );
        nlr.setWeightedSumVariable( 3, 1, 13 );

        // Mark nodes as ReLUs
        nlr.setNeuronActivationFunction( 1, 0, NetworkLevelReasoner::ReLU );
        nlr.setNeuronActivationFunction( 1, 1, NetworkLevelReasoner::ReLU );
        nlr.setNeuronActivationFunction( 1, 2, NetworkLevelReasoner::ReLU );

        nlr.setNeuronActivationFunction( 2, 0, NetworkLevelReasoner::ReLU );
        nlr.setNeuronActivationFunction( 2, 1, NetworkLevelReasoner::ReLU );
    }

    void test_evaluate()
    {
        NetworkLevelReasoner nlr;

        populateNetwork( nlr );

        double input[2];
        double output[2];

        // With ReLUs, Inputs are zeros, only biases count
        input[0] = 0;
        input[1] = 0;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 1 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 4 ) );

        // With ReLUs, case 1
        input[0] = 1;
        input[1] = 1;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 1 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 1 ) );

        // With ReLUs, case 2
        input[0] = 1;
        input[1] = 2;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 0 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 0 ) );
    }

    void test_store_into_other()
    {
        NetworkLevelReasoner nlr;

        populateNetwork( nlr );

        NetworkLevelReasoner nlr2;

        TS_ASSERT_THROWS_NOTHING( nlr.storeIntoOther( nlr2 ) );

        double input[2];
        double output1[2];
        double output2[2];

        // Inputs are zeros, only biases count
        input[0] = 0;
        input[1] = 0;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output1 ) );
        TS_ASSERT_THROWS_NOTHING( nlr2.evaluate( input, output2 ) );

        TS_ASSERT( FloatUtils::areEqual( output1[0], output2[0] ) );
        TS_ASSERT( FloatUtils::areEqual( output1[1], output2[1] ) );

        // Set all neurons to ReLU, except for input and output neurons
        nlr.setNeuronActivationFunction( 1, 0, NetworkLevelReasoner::ReLU );
        nlr.setNeuronActivationFunction( 1, 1, NetworkLevelReasoner::ReLU );
        nlr.setNeuronActivationFunction( 1, 2, NetworkLevelReasoner::ReLU );

        nlr.setNeuronActivationFunction( 2, 0, NetworkLevelReasoner::ReLU );
        nlr.setNeuronActivationFunction( 2, 1, NetworkLevelReasoner::ReLU );

        TS_ASSERT_THROWS_NOTHING( nlr.storeIntoOther( nlr2 ) );

        // With ReLUs, Inputs are zeros, only biases count
        input[0] = 0;
        input[1] = 0;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output1 ) );
        TS_ASSERT_THROWS_NOTHING( nlr2.evaluate( input, output2 ) );

        TS_ASSERT( FloatUtils::areEqual( output1[0], output2[0] ) );
        TS_ASSERT( FloatUtils::areEqual( output1[1], output2[1] ) );

        // With ReLUs, case 1
        input[0] = 1;
        input[1] = 1;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output1 ) );
        TS_ASSERT_THROWS_NOTHING( nlr2.evaluate( input, output2 ) );

        TS_ASSERT( FloatUtils::areEqual( output1[0], output2[0] ) );
        TS_ASSERT( FloatUtils::areEqual( output1[1], output2[1] ) );
    }

    void test_interval_arithmetic_bound_propagation()
    {
        NetworkLevelReasoner nlr;
        populateNetwork( nlr );

        MockTableau tableau;

        // Initialize the bounds
        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        double large = 1000;
        tableau.setLowerBound( 2, -large ); tableau.setUpperBound( 2, large );
        tableau.setLowerBound( 3, -large ); tableau.setUpperBound( 3, large );
        tableau.setLowerBound( 4, -large ); tableau.setUpperBound( 4, large );
        tableau.setLowerBound( 5, -large ); tableau.setUpperBound( 5, large );
        tableau.setLowerBound( 6, -large ); tableau.setUpperBound( 6, large );
        tableau.setLowerBound( 7, -large ); tableau.setUpperBound( 7, large );
        tableau.setLowerBound( 8, -large ); tableau.setUpperBound( 8, large );
        tableau.setLowerBound( 9, -large ); tableau.setUpperBound( 9, large );
        tableau.setLowerBound( 10, -large ); tableau.setUpperBound( 10, large );
        tableau.setLowerBound( 11, -large ); tableau.setUpperBound( 11, large );
        tableau.setLowerBound( 12, -large ); tableau.setUpperBound( 12, large );
        tableau.setLowerBound( 13, -large ); tableau.setUpperBound( 13, large );

        nlr.setTableau( &tableau );

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds({
                Tightening( 2, 0, Tightening::LB ),
                Tightening( 2, 2, Tightening::UB ),
                Tightening( 3, 0, Tightening::LB ),
                Tightening( 3, 2, Tightening::UB ),

                Tightening( 4, -5, Tightening::LB ),
                Tightening( 4, 5, Tightening::UB ),
                Tightening( 5, 0, Tightening::LB ),
                Tightening( 5, 5, Tightening::UB ),

                Tightening( 6, -1, Tightening::LB ),
                Tightening( 6, 1, Tightening::UB ),
                Tightening( 7, 0, Tightening::LB ),
                Tightening( 7, 1, Tightening::UB ),

                Tightening( 8, -1, Tightening::LB ),
                Tightening( 8, 7, Tightening::UB ),
                Tightening( 9, 0, Tightening::LB ),
                Tightening( 9, 7, Tightening::UB ),

                Tightening( 10, -1, Tightening::LB ),
                Tightening( 10, 7, Tightening::UB ),
                Tightening( 11, 0, Tightening::LB ),
                Tightening( 11, 7, Tightening::UB ),

                Tightening( 12, 0, Tightening::LB ),
                Tightening( 12, 7, Tightening::UB ),
                Tightening( 13, 0, Tightening::LB ),
                Tightening( 13, 28, Tightening::UB ),
                    });

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT_EQUALS( expectedBounds, bounds );

        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

        tableau.setLowerBound( 2, -large ); tableau.setUpperBound( 2, large );
        tableau.setLowerBound( 3, -large ); tableau.setUpperBound( 3, large );
        tableau.setLowerBound( 4, -large ); tableau.setUpperBound( 4, large );
        tableau.setLowerBound( 5, -large ); tableau.setUpperBound( 5, large );
        tableau.setLowerBound( 6, -large ); tableau.setUpperBound( 6, large );
        tableau.setLowerBound( 7, -large ); tableau.setUpperBound( 7, large );
        tableau.setLowerBound( 8, -large ); tableau.setUpperBound( 8, large );
        tableau.setLowerBound( 9, -large ); tableau.setUpperBound( 9, large );
        tableau.setLowerBound( 10, -large ); tableau.setUpperBound( 10, large );
        tableau.setLowerBound( 11, -large ); tableau.setUpperBound( 11, large );
        tableau.setLowerBound( 12, -large ); tableau.setUpperBound( 12, large );
        tableau.setLowerBound( 13, -large ); tableau.setUpperBound( 13, large );

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds2({
                Tightening( 2, -2, Tightening::LB ),
                Tightening( 2, 2, Tightening::UB ),
                Tightening( 3, 0, Tightening::LB ),
                Tightening( 3, 2, Tightening::UB ),

                Tightening( 4, -12, Tightening::LB ),
                Tightening( 4, 5, Tightening::UB ),
                Tightening( 5, 0, Tightening::LB ),
                Tightening( 5, 5, Tightening::UB ),

                Tightening( 6, -1, Tightening::LB ),
                Tightening( 6, 2, Tightening::UB ),
                Tightening( 7, 0, Tightening::LB ),
                Tightening( 7, 2, Tightening::UB ),

                Tightening( 8, -2, Tightening::LB ),
                Tightening( 8, 7, Tightening::UB ),
                Tightening( 9, 0, Tightening::LB ),
                Tightening( 9, 7, Tightening::UB ),

                Tightening( 10, -2, Tightening::LB ),
                Tightening( 10, 7, Tightening::UB ),
                Tightening( 11, 0, Tightening::LB ),
                Tightening( 11, 7, Tightening::UB ),

                Tightening( 12, 0, Tightening::LB ),
                Tightening( 12, 7, Tightening::UB ),
                Tightening( 13, 0, Tightening::LB ),
                Tightening( 13, 28, Tightening::UB ),
                    });

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT_EQUALS( expectedBounds2, bounds );
    }

    void test_symbolic_bound_tightening()
    {
        NetworkLevelReasoner nlr;

        nlr.setNumberOfLayers( 3 );

        nlr.setLayerSize( 0, 2 );
        nlr.setLayerSize( 1, 2 );
        nlr.setLayerSize( 2, 1 );

        nlr.allocateMemoryByTopology();

        // Weights
        nlr.setWeight( 0, 0, 0, 2 );
        nlr.setWeight( 0, 0, 1, 1 );
        nlr.setWeight( 0, 1, 0, 3 );
        nlr.setWeight( 0, 1, 1, 1 );
        nlr.setWeight( 1, 0, 0, 1 );
        nlr.setWeight( 1, 1, 0, -1 );

        // All biases are 0
        nlr.setBias( 0, 0, 0 );
        nlr.setBias( 0, 1, 0 );
        nlr.setBias( 1, 0, 0 );
        nlr.setBias( 1, 1, 0 );
        nlr.setBias( 2, 0, 0 );

        // Variable indexing
        nlr.setActivationResultVariable( 0, 0, 0 );
        nlr.setActivationResultVariable( 0, 1, 1 );

        nlr.setWeightedSumVariable( 1, 0, 2 );
        nlr.setWeightedSumVariable( 1, 1, 3 );
        nlr.setActivationResultVariable( 1, 0, 4 );
        nlr.setActivationResultVariable( 1, 1, 5 );

        nlr.setWeightedSumVariable( 2, 0, 6 );

        // Mark nodes as ReLUs
        nlr.setNeuronActivationFunction( 1, 0, NetworkLevelReasoner::ReLU );
        nlr.setNeuronActivationFunction( 1, 1, NetworkLevelReasoner::ReLU );

        // Prepare initial bounds
        MockTableau tableau;
        nlr.setTableau( &tableau );

        double large = 1000000;

        tableau.setLowerBound( 0, 4 );
        tableau.setUpperBound( 0, 6 );
        tableau.setLowerBound( 1, 1 );
        tableau.setUpperBound( 1, 5 );

        tableau.setLowerBound( 2, -large ); tableau.setUpperBound( 2, large );
        tableau.setLowerBound( 3, -large ); tableau.setUpperBound( 3, large );
        tableau.setLowerBound( 4, -large ); tableau.setUpperBound( 4, large );
        tableau.setLowerBound( 5, -large ); tableau.setUpperBound( 5, large );
        tableau.setLowerBound( 6, -large ); tableau.setUpperBound( 6, large );

        // Invoke SBT
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.symbolicBoundPropagation() );

        /*
          Input ranges:

          x0: [4, 6]
          x1: [1, 5]

          Layer 1:

          x2.lb = 2x0 + 3x1   : [11, 27]
          x2.ub = 2x0 + 3x1   : [11, 27]

          x3.lb =  x0 +  x1   : [5, 11]
          x3.ub =  x0 +  x1   : [5, 11]

          Both ReLUs active, bound survive through activations:

          x4.lb = 2x0 + 3x1   : [11, 27]
          x4.ub = 2x0 + 3x1   : [11, 27]

          x5.lb =  x0 +  x1   : [5, 11]
          x5.ub =  x0 +  x1   : [5, 11]

          Layer 2:

          x6.lb =  x0 + 2x1   : [6, 16]
          x6.ub =  x0 + 2x1   : [6, 16]
        */

        List<Tightening> expectedBounds({
                Tightening( 2, 11, Tightening::LB ),
                Tightening( 2, 27, Tightening::UB ),
                Tightening( 3, 5, Tightening::LB ),
                Tightening( 3, 11, Tightening::UB ),

                Tightening( 4, 11, Tightening::LB ),
                Tightening( 4, 27, Tightening::UB ),
                Tightening( 5, 5, Tightening::LB ),
                Tightening( 5, 11, Tightening::UB ),

                Tightening( 6, 6, Tightening::LB ),
                Tightening( 6, 16, Tightening::UB ),
                    });

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : bounds )
            TS_ASSERT( expectedBounds.exists( bound ) );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
