/*********************                                                        */
/*! \file Test_CEGARSolver.h
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

#include "CEGARSolver.h"
#include "MockErrno.h"
#include "NetworkLevelReasoner.h"

#include <string.h>

class MockForCEGARSolver
    : public MockErrno
{
public:
};

class CEGARSolverTestSuite : public CxxTest::TestSuite
{
public:
    MockForCEGARSolver *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForCEGARSolver );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    InputQuery prepareInputQuery()
    {
        InputQuery ipq;

        /*
          Prepare a simple ReLU network as an ipq

          Layer 0 (input):
            x0, x1

          Layer 1 (WS):
            x2, x3

          Layer 2 (ReLU):
            x4, x5

          Layer 3 (WS):
            x6, x7, x8

          Layer 4 (ReLU):
            x9, x10, x11

          Layer 5 (WS):
            x12, x13

          Layer 6 (ReLU):
            x14, x15

          Layer 7 (WS, output):
            x16
        */

        ipq.setNumberOfVariables( 17 );

        // Layer 0, input
        ipq.markInputVariable( 0, 0 );
        ipq.markInputVariable( 1, 1 );

        ipq.setLowerBound( 0, -1 );
        ipq.setUpperBound( 0, 1 );
        ipq.setLowerBound( 1, -1 );
        ipq.setUpperBound( 1, 1 );

        // Layer 1, WS
        // x2 = x0 + 2x1 + 1
        Equation eq0;
        eq0.addAddend( 1, 0 );
        eq0.addAddend( 2, 1 );
        eq0.addAddend( -1, 2 );
        eq0.setScalar( -1 );
        ipq.addEquation( eq0 );

        // x3 = -x0 + x1 + 2
        Equation eq1;
        eq1.addAddend( -1, 0 );
        eq1.addAddend( 1, 1 );
        eq1.addAddend( -1, 3 );
        eq1.setScalar( -2 );
        ipq.addEquation( eq1 );

        // Layer 2, ReLU
        ReluConstraint *relu0 = new ReluConstraint( 2, 4 );
        ipq.addPiecewiseLinearConstraint( relu0 );

        ReluConstraint *relu1 = new ReluConstraint( 3, 5 );
        ipq.addPiecewiseLinearConstraint( relu1 );

        // Layer 3, WS
        // x6 = x4 + 2x5 + 3
        Equation eq2;
        eq2.addAddend( 1, 4 );
        eq2.addAddend( 2, 5 );
        eq2.addAddend( -1, 6 );
        eq2.setScalar( -3 );
        ipq.addEquation( eq2 );

        // x7 = -x4 - 2
        Equation eq3;
        eq3.addAddend( -1, 4 );
        eq3.addAddend( -1, 7 );
        eq3.setScalar( 2 );
        ipq.addEquation( eq3 );

        Equation eq4;
        // x8 = 3x - 4
        eq4.addAddend( 3, 5 );
        eq4.addAddend( -1, 8 );
        eq4.setScalar( 4 );
        ipq.addEquation( eq4 );

        // Layer 4, ReLU
        ReluConstraint *relu2 = new ReluConstraint( 6, 9 );
        ipq.addPiecewiseLinearConstraint( relu2 );

        ReluConstraint *relu3 = new ReluConstraint( 7, 10 );
        ipq.addPiecewiseLinearConstraint( relu3 );

        ReluConstraint *relu4 = new ReluConstraint( 8, 11 );
        ipq.addPiecewiseLinearConstraint( relu4 );

        // Layer 5, WS
        // x12 = x9 -2x10 + 3x11 + 2
        Equation eq5;
        eq5.addAddend( 1, 9 );
        eq5.addAddend( -2, 10 );
        eq5.addAddend( 3, 11 );
        eq5.addAddend( -1, 12 );
        eq5.setScalar( -2 );
        ipq.addEquation( eq5 );

        // x13 = -x9 +2x10 - 3x11 -3
        Equation eq6;
        eq6.addAddend( -1, 9 );
        eq6.addAddend( 2, 10 );
        eq6.addAddend( -3, 11 );
        eq6.addAddend( -1, 13 );
        eq6.setScalar( 3 );
        ipq.addEquation( eq6 );

        // Layer 6, ReLU
        ReluConstraint *relu5 = new ReluConstraint( 12, 14 );
        ipq.addPiecewiseLinearConstraint( relu5 );

        ReluConstraint *relu6 = new ReluConstraint( 13, 15 );
        ipq.addPiecewiseLinearConstraint( relu6 );

        // Layer 7: output
        // x16 = x14 - 2x15
        Equation eq7;
        eq7.addAddend( 1, 14 );
        eq7.addAddend( -2, 15 );
        eq7.addAddend( -1, 16 );
        eq7.setScalar( 0 );
        ipq.addEquation( eq7 );

        ipq.markOutputVariable( 16, 0 );

        ipq.constructNetworkLevelReasoner();

        // ipq.getNetworkLevelReasoner()->dumpTopology();

        return ipq;
    }

    void test_preprocess_query()
    {
        InputQuery ipq = prepareInputQuery();
        NLR::NetworkLevelReasoner *nlr = ipq.getNetworkLevelReasoner();

        CEGARSolver cegar;

        TS_ASSERT_THROWS_NOTHING( cegar.storeBaseQuery( ipq ) );
        TS_ASSERT_THROWS_NOTHING( cegar.preprocessQuery() );

        InputQuery ppIpq = cegar.getPreprocessedQuery();
        TS_ASSERT( ppIpq.getNetworkLevelReasoner() );
        NLR::NetworkLevelReasoner *ppNlr = ppIpq.getNetworkLevelReasoner();

        // Number of layers is unchanged
        TS_ASSERT_EQUALS( nlr->getNumberOfLayers(), ppNlr->getNumberOfLayers() );

        // Only one output neuron
        const NLR::Layer *lastLayer = ppNlr->getLayer( ppNlr->getNumberOfLayers() - 1 );
        TS_ASSERT_EQUALS( lastLayer->getSize(), 1U );
        TS_ASSERT_EQUALS( lastLayer->getLayerType(), NLR::Layer::WEIGHTED_SUM );

        // Layers 5 and 6, WS + RELU
        const NLR::Layer *layer5 = ppNlr->getLayer( 5 );
        const NLR::Layer *layer6 = ppNlr->getLayer( 6 );

        TS_ASSERT_EQUALS( layer5->getLayerType(), NLR::Layer::WEIGHTED_SUM );
        TS_ASSERT_EQUALS( layer6->getLayerType(), NLR::Layer::RELU );

        // Each neuron duplicated 4 times
        TS_ASSERT_EQUALS( layer5->getSize(), 2U * 4 );
        TS_ASSERT_EQUALS( layer6->getSize(), 2U * 4 );

        /*
          Expected PP layer sizes:

          Layer 0: 2    Input
          Layer 1: 8    WS
          Layer 2: 8    Relu
          Layer 3: 12   WS
          Layer 4: 12   Relu
          Layer 5: 8    WS
          Layer 6: 8    Relu
          Layer 7: 1    Output
        */

        /*
          x14 previously had a positive outgoing edge to x16, so only
          its <pos,inc> copy should retain this edge. x15 had a
          negative edge, so only its <neg,dec> should retain it.
        */
        TS_ASSERT_EQUALS( lastLayer->getWeight( 6, 0, 0 ), 1 );   // POS INC
        TS_ASSERT_EQUALS( lastLayer->getWeight( 6, 1, 0 ), 0 );
        TS_ASSERT_EQUALS( lastLayer->getWeight( 6, 2, 0 ), 0 );
        TS_ASSERT_EQUALS( lastLayer->getWeight( 6, 3, 0 ), 0 );

        TS_ASSERT_EQUALS( lastLayer->getWeight( 6, 4, 0 ), 0 );
        TS_ASSERT_EQUALS( lastLayer->getWeight( 6, 5, 0 ), 0 );
        TS_ASSERT_EQUALS( lastLayer->getWeight( 6, 6, 0 ), -2 );  // NEG DEC
        TS_ASSERT_EQUALS( lastLayer->getWeight( 6, 7, 0 ), 0 );

        TS_ASSERT_EQUALS( lastLayer->getBias( 0 ), 0 );

        // Each relu neuron is mapped to matching node
        for ( unsigned i = 0; i < 8; ++i )
        {
            TS_ASSERT_EQUALS( layer6->getActivationSources( i ).size(), 1U );
            TS_ASSERT_EQUALS( layer6->getActivationSources( i ).begin()->_layer, 5U );
            TS_ASSERT_EQUALS( layer6->getActivationSources( i ).begin()->_neuron, i );
        }

        /*
          Check the edges from layer 4 to 5
        */

        // Edge from x9 to x12: 1. POS neurons only,
        // INC feeds into INC, DEC into DEC
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 0, 0 ), 1 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 1, 0 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 2, 0 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 3, 0 ), 0 );

        TS_ASSERT_EQUALS( layer5->getWeight( 4, 0, 1 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 1, 1 ), 1 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 2, 1 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 3, 1 ), 0 );

        TS_ASSERT_EQUALS( layer5->getWeight( 4, 0, 2 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 1, 2 ), 1 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 2, 2 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 3, 2 ), 0 );

        TS_ASSERT_EQUALS( layer5->getWeight( 4, 0, 3 ), 1 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 1, 3 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 2, 3 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 3, 3 ), 0 );

        // Edge from x9 to x13: -1. NEG neurons only,
        // INC feeds into DEC, DEC into INC
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 0, 4 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 1, 4 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 2, 4 ), -1 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 3, 4 ), 0 );

        TS_ASSERT_EQUALS( layer5->getWeight( 4, 0, 5 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 1, 5 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 2, 5 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 3, 5 ), -1 );

        TS_ASSERT_EQUALS( layer5->getWeight( 4, 0, 6 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 1, 6 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 2, 6 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 3, 6 ), -1 );

        TS_ASSERT_EQUALS( layer5->getWeight( 4, 0, 7 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 1, 7 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 2, 7 ), -1 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 3, 7 ), 0 );

        // Edge from x10 to x12: -2.
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 4, 0 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 5, 0 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 6, 0 ), -2 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 7, 0 ), 0 );

        TS_ASSERT_EQUALS( layer5->getWeight( 4, 4, 1 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 5, 1 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 6, 1 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 7, 1 ), -2 );

        TS_ASSERT_EQUALS( layer5->getWeight( 4, 4, 2 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 5, 2 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 6, 2 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 7, 2 ), -2 );

        TS_ASSERT_EQUALS( layer5->getWeight( 4, 4, 3 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 5, 3 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 6, 3 ), -2 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 7, 3 ), 0 );

        // Edge from x10 to x13: 2.
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 4, 4 ), 2 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 5, 4 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 6, 4 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 7, 4 ), 0 );

        TS_ASSERT_EQUALS( layer5->getWeight( 4, 4, 5 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 5, 5 ), 2 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 6, 5 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 7, 5 ), 0 );

        TS_ASSERT_EQUALS( layer5->getWeight( 4, 4, 6 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 5, 6 ), 2 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 6, 6 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 7, 6 ), 0 );

        TS_ASSERT_EQUALS( layer5->getWeight( 4, 4, 7 ), 2 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 5, 7 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 6, 7 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 7, 7 ), 0 );

        // Edge from x11 to x12: 3.
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 8, 0 ), 3 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 9, 0 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 10, 0 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 11, 0 ), 0 );

        TS_ASSERT_EQUALS( layer5->getWeight( 4, 8, 1 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 9, 1 ), 3 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 10, 1 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 11, 1 ), 0 );

        TS_ASSERT_EQUALS( layer5->getWeight( 4, 8, 2 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 9, 2 ), 3 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 10, 2 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 11, 2 ), 0 );

        TS_ASSERT_EQUALS( layer5->getWeight( 4, 8, 3 ), 3 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 9, 3 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 10, 3 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 11, 3 ), 0 );

        // Edge from x9 to x13: -3.
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 8, 4 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 9, 4 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 10, 4 ), -3 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 11, 4 ), 0 );

        TS_ASSERT_EQUALS( layer5->getWeight( 4, 8, 5 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 9, 5 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 10, 5 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 11, 5 ), -3 );

        TS_ASSERT_EQUALS( layer5->getWeight( 4, 8, 6 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 9, 6 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 10, 6 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 11, 6 ), -3 );

        TS_ASSERT_EQUALS( layer5->getWeight( 4, 8, 7 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 9, 7 ), 0 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 10, 7 ), -3 );
        TS_ASSERT_EQUALS( layer5->getWeight( 4, 11, 7 ), 0 );

        // Biases
        TS_ASSERT_EQUALS( layer5->getBias( 0 ), 2 );
        TS_ASSERT_EQUALS( layer5->getBias( 1 ), 2 );
        TS_ASSERT_EQUALS( layer5->getBias( 2 ), 2 );
        TS_ASSERT_EQUALS( layer5->getBias( 3 ), 2 );
        TS_ASSERT_EQUALS( layer5->getBias( 4 ), -3 );
        TS_ASSERT_EQUALS( layer5->getBias( 5 ), -3 );
        TS_ASSERT_EQUALS( layer5->getBias( 6 ), -3 );
        TS_ASSERT_EQUALS( layer5->getBias( 7 ), -3 );

        // Layers 3 and 4, WS + RELU
        const NLR::Layer *layer3 = ppNlr->getLayer( 3 );
        const NLR::Layer *layer4 = ppNlr->getLayer( 4 );

        TS_ASSERT_EQUALS( layer3->getLayerType(), NLR::Layer::WEIGHTED_SUM );
        TS_ASSERT_EQUALS( layer4->getLayerType(), NLR::Layer::RELU );

        // Each neuron duplicated 4 times
        TS_ASSERT_EQUALS( layer3->getSize(), 3U * 4 );
        TS_ASSERT_EQUALS( layer4->getSize(), 3U * 4 );

        // Each relu neuron is mapped to matching node
        for ( unsigned i = 0; i < 12; ++i )
        {
            TS_ASSERT_EQUALS( layer4->getActivationSources( i ).size(), 1U );
            TS_ASSERT_EQUALS( layer4->getActivationSources( i ).begin()->_layer, 3U );
            TS_ASSERT_EQUALS( layer4->getActivationSources( i ).begin()->_neuron, i );
        }

        // Edge from x4 to x6: 1.
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 0, 0 ), 1 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 1, 0 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 2, 0 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 3, 0 ), 0 );

        TS_ASSERT_EQUALS( layer3->getWeight( 2, 0, 1 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 1, 1 ), 1 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 2, 1 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 3, 1 ), 0 );

        TS_ASSERT_EQUALS( layer3->getWeight( 2, 0, 2 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 1, 2 ), 1 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 2, 2 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 3, 2 ), 0 );

        TS_ASSERT_EQUALS( layer3->getWeight( 2, 0, 3 ), 1 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 1, 3 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 2, 3 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 3, 3 ), 0 );

        // Edge from x4 to x7: -1.
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 0, 4 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 1, 4 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 2, 4 ), -1 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 3, 4 ), 0 );

        TS_ASSERT_EQUALS( layer3->getWeight( 2, 0, 5 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 1, 5 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 2, 5 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 3, 5 ), -1 );

        TS_ASSERT_EQUALS( layer3->getWeight( 2, 0, 6 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 1, 6 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 2, 6 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 3, 6 ), -1 );

        TS_ASSERT_EQUALS( layer3->getWeight( 2, 0, 7 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 1, 7 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 2, 7 ), -1 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 3, 7 ), 0 );

        // Edge from x4 to x8: 0
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 0, 8 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 1, 8 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 2, 8 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 3, 8 ), 0 );

        TS_ASSERT_EQUALS( layer3->getWeight( 2, 0, 9 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 1, 9 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 2, 9 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 3, 9 ), 0 );

        TS_ASSERT_EQUALS( layer3->getWeight( 2, 0, 10 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 1, 10 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 2, 10 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 3, 10 ), 0 );

        TS_ASSERT_EQUALS( layer3->getWeight( 2, 0, 11 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 1, 11 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 2, 11 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 3, 11 ), 0 );

        // Edge from x5 to x6: 2.
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 4, 0 ), 2 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 5, 0 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 6, 0 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 7, 0 ), 0 );

        TS_ASSERT_EQUALS( layer3->getWeight( 2, 4, 2 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 5, 2 ), 2 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 6, 2 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 7, 2 ), 0 );

        TS_ASSERT_EQUALS( layer3->getWeight( 2, 4, 2 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 5, 2 ), 2 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 6, 2 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 7, 2 ), 0 );

        TS_ASSERT_EQUALS( layer3->getWeight( 2, 4, 3 ), 2 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 5, 3 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 6, 3 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 7, 3 ), 0 );

        // Edge from x5 to x7: 0.
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 4, 4 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 5, 4 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 6, 4 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 7, 4 ), 0 );

        TS_ASSERT_EQUALS( layer3->getWeight( 2, 4, 5 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 5, 5 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 6, 5 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 7, 5 ), 0 );

        TS_ASSERT_EQUALS( layer3->getWeight( 2, 4, 6 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 5, 6 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 6, 6 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 7, 6 ), 0 );

        TS_ASSERT_EQUALS( layer3->getWeight( 2, 4, 7 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 5, 7 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 6, 7 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 7, 7 ), 0 );

        // Edge from x5 to x8: 3
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 4, 8 ), 3 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 5, 8 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 6, 8 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 7, 8 ), 0 );

        TS_ASSERT_EQUALS( layer3->getWeight( 2, 4, 9 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 5, 9 ), 3 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 6, 9 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 7, 9 ), 0 );

        TS_ASSERT_EQUALS( layer3->getWeight( 2, 4, 10 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 5, 10 ), 3 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 6, 10 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 7, 10 ), 0 );

        TS_ASSERT_EQUALS( layer3->getWeight( 2, 4, 11 ), 3 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 5, 11 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 6, 11 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 7, 11 ), 0 );

        // Layers 1 and 2, WS + RELU
        const NLR::Layer *layer1 = ppNlr->getLayer( 1 );
        const NLR::Layer *layer2 = ppNlr->getLayer( 2 );

        TS_ASSERT_EQUALS( layer1->getLayerType(), NLR::Layer::WEIGHTED_SUM );
        TS_ASSERT_EQUALS( layer2->getLayerType(), NLR::Layer::RELU );

        // Each neuron duplicated 4 times
        TS_ASSERT_EQUALS( layer1->getSize(), 2U * 4 );
        TS_ASSERT_EQUALS( layer2->getSize(), 2U * 4 );

        // Each relu neuron is mapped to matching node
        for ( unsigned i = 0; i < 8; ++i )
        {
            TS_ASSERT_EQUALS( layer2->getActivationSources( i ).size(), 1U );
            TS_ASSERT_EQUALS( layer2->getActivationSources( i ).begin()->_layer, 1U );
            TS_ASSERT_EQUALS( layer2->getActivationSources( i ).begin()->_neuron, i );
        }

        // Edge from x0 to x2: 1.
        TS_ASSERT_EQUALS( layer1->getWeight( 0, 0, 0 ), 1 );
        TS_ASSERT_EQUALS( layer1->getWeight( 0, 0, 1 ), 1 );
        TS_ASSERT_EQUALS( layer1->getWeight( 0, 0, 2 ), 1 );
        TS_ASSERT_EQUALS( layer1->getWeight( 0, 0, 3 ), 1 );

        // Edge from x0 to x3: -1.
        TS_ASSERT_EQUALS( layer1->getWeight( 0, 0, 4 ), -1 );
        TS_ASSERT_EQUALS( layer1->getWeight( 0, 0, 5 ), -1 );
        TS_ASSERT_EQUALS( layer1->getWeight( 0, 0, 6 ), -1 );
        TS_ASSERT_EQUALS( layer1->getWeight( 0, 0, 7 ), -1 );

        // Edge from x1 to x2: 2.
        TS_ASSERT_EQUALS( layer1->getWeight( 0, 1, 0 ), 2 );
        TS_ASSERT_EQUALS( layer1->getWeight( 0, 1, 1 ), 2 );
        TS_ASSERT_EQUALS( layer1->getWeight( 0, 1, 2 ), 2 );
        TS_ASSERT_EQUALS( layer1->getWeight( 0, 1, 3 ), 2 );

        // Edge from x1 to x3: 1.
        TS_ASSERT_EQUALS( layer1->getWeight( 0, 1, 4 ), 1 );
        TS_ASSERT_EQUALS( layer1->getWeight( 0, 1, 5 ), 1 );
        TS_ASSERT_EQUALS( layer1->getWeight( 0, 1, 6 ), 1 );
        TS_ASSERT_EQUALS( layer1->getWeight( 0, 1, 7 ), 1 );

        TS_ASSERT_EQUALS( ppNlr->getLayer( 0 )->getSize(),
                          nlr->getLayer( 0 )->getSize() );

        // Check that the original network and the preprocessed one
        // give the same output on some inputs

        double input1[2] = { 0.5, -2 };
        double input2[2] = { 0, 10 };
        double input3[2] = { -3, 2.5 };

        double output;
        double ppOutput;

        nlr->evaluate( input1, &output );
        ppNlr->evaluate( input1, &ppOutput );
        TS_ASSERT_EQUALS( output, ppOutput );

        nlr->evaluate( input2, &output );
        ppNlr->evaluate( input2, &ppOutput );
        TS_ASSERT_EQUALS( output, ppOutput );

        nlr->evaluate( input3, &output );
        ppNlr->evaluate( input3, &ppOutput );
        TS_ASSERT_EQUALS( output, ppOutput );
    }

    void test_create_intial_abstraction()
    {
        InputQuery ipq = prepareInputQuery();
        // NLR::NetworkLevelReasoner *nlr = ipq.getNetworkLevelReasoner();

        CEGARSolver cegar;

        TS_ASSERT_THROWS_NOTHING( cegar.storeBaseQuery( ipq ) );
        TS_ASSERT_THROWS_NOTHING( cegar.preprocessQuery() );

        InputQuery ppIpq = cegar.getPreprocessedQuery();
        TS_ASSERT( ppIpq.getNetworkLevelReasoner() );
        NLR::NetworkLevelReasoner *ppNlr = ppIpq.getNetworkLevelReasoner();

        TS_TRACE( "Starting" );
        TS_ASSERT_THROWS_NOTHING( cegar.createInitialAbstraction() );
        TS_TRACE( "Done" );

        InputQuery abstractQuery = cegar.getCurrentQuery();

        if ( !abstractQuery.getNetworkLevelReasoner() )
            abstractQuery.constructNetworkLevelReasoner();

        NLR::NetworkLevelReasoner *absNlr = abstractQuery.getNetworkLevelReasoner();

        // Number of layers is unchanged
        // TS_ASSERT_EQUALS( nlr->getNumberOfLayers(), ppNlr->getNumberOfLayers() );

        // // Only one output neuron
        // const NLR::Layer *lastLayer = ppNlr->getLayer( ppNlr->getNumberOfLayers() - 1 );
        // TS_ASSERT_EQUALS( lastLayer->getSize(), 1U );
        // TS_ASSERT_EQUALS( lastLayer->getLayerType(), NLR::Layer::WEIGHTED_SUM );

        // Layers 0, 1 and 2 should be identical
        TS_ASSERT( ( *ppNlr->getLayer( 0 ) ) == ( *absNlr->getLayer( 0 ) ) );
        TS_ASSERT( ( *ppNlr->getLayer( 1 ) ) == ( *absNlr->getLayer( 1 ) ) );
        TS_ASSERT( ( *ppNlr->getLayer( 2 ) ) == ( *absNlr->getLayer( 2 ) ) );

        // Layer 3: 4 nodes, weights determined by mins and maxes
        const NLR::Layer *layer3 = absNlr->getLayer( 3 );

        TS_ASSERT_EQUALS( layer3->getSize(), 4U );
        TS_ASSERT_EQUALS( layer3->getLayerType(), NLR::Layer::WEIGHTED_SUM );

        // Edges into x18, which is the POS,INC node of layer 3
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 0, 0 ), 1 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 1, 0 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 2, 0 ), -1 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 3, 0 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 4, 0 ), 3 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 5, 0 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 6, 0 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 7, 0 ), 0 );

        // Edges into x19, which is the POS,DEC node of layer 3
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 0, 1 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 1, 1 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 2, 1 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 3, 1 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 4, 1 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 5, 1 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 6, 1 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 7, 1 ), 0 );

        // Edges into x20, which is the NEG,DEC node of layer 3
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 0, 2 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 1, 2 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 2, 2 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 3, 2 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 4, 2 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 5, 2 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 6, 2 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 7, 2 ), 0 );

        // Edges into x21, which is the NEG,INC node of layer 3
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 0, 3 ), 1 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 1, 3 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 2, 3 ), -1 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 3, 3 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 4, 3 ), 3 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 5, 3 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 6, 3 ), 0 );
        TS_ASSERT_EQUALS( layer3->getWeight( 2, 7, 3 ), 0 );

        // Check the biases
        TS_ASSERT_EQUALS( layer3->getBias( 0 ), 3 );
        TS_ASSERT_EQUALS( layer3->getBias( 1 ), -4 );
        TS_ASSERT_EQUALS( layer3->getBias( 2 ), -4 );
        TS_ASSERT_EQUALS( layer3->getBias( 3 ), 3 );

        // Layer 4: 4 nodes, ReLUs of previous layer
        const NLR::Layer *layer4 = absNlr->getLayer( 4 );

        TS_ASSERT_EQUALS( layer4->getSize(), 4U );
        TS_ASSERT_EQUALS( layer4->getLayerType(), NLR::Layer::RELU );

        // Each relu neuron is mapped to matching node
        for ( unsigned i = 0; i < 4; ++i )
        {
            TS_ASSERT_EQUALS( layer4->getActivationSources( i ).size(), 1U );
            TS_ASSERT_EQUALS( layer4->getActivationSources( i ).begin()->_layer, 3U );
            TS_ASSERT_EQUALS( layer4->getActivationSources( i ).begin()->_neuron, i );
        }

        layer3->dump();
    }
};
