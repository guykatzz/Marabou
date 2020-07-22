/*********************                                                        */
/*! \file CEGARSolver.h
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

#ifndef __CEGARSolver_h__
#define __CEGARSolver_h__

#include "Engine.h"
#include "InputQuery.h"
#include "Layer.h"
#include "Options.h"

class CEGARSolver
{
public:

    void run( InputQuery &query )
    {
        printf( "CEGARSolver::run called\n" );

        storeBaseQuery( query );

        preprocessQuery();

        printf( "Preprocessing is complete!\n" );

        createInitialAbstraction();

        exit( 1 );

        while ( true )
        {
            solve();

            if ( sat() && !spurious() )
            {
                // Truly SAT
                printf( "\n\nSAT\n" );
                return;
            }
            else if ( unsat() )
            {
                // Truly UNSAT
                printf( "\n\nUNSAT\n" );
                return;
            }

            refine();
        }
    }

private:
    /*
      The original query provided by the user
    */
    InputQuery _baseQuery;

    /*
      The preprocessed query; equivalent to _baseQuery, roughly 4
      times larger
    */
    InputQuery _preprocessedQuery;

    InputQuery _currentQuery;
    Engine::ExitCode _engineExitCode;

    enum NeuronType {
        POS_INC = 0,
        POS_DEC = 1,
        NEG_DEC = 2,
        NEG_INC = 3,
    };

    enum WeightOperator {
       ZERO = 0,
       MAX = 1,
       MIN = 2,
    };

    Map<NeuronType, Map<NeuronType,unsigned>> _weightOperators;

    void initializeWeightOperators()
    {
        // Edges outgoing from a POS_INC neuron
        _weightOperators[POS_INC][POS_INC] = MAX;
        _weightOperators[POS_INC][POS_DEC] = ZERO;
        _weightOperators[POS_INC][NEG_DEC] = ZERO;
        _weightOperators[POS_INC][NEG_INC] = MAX;

        // Edges outgoing from a POS_DEC neuron
        _weightOperators[POS_DEC][POS_INC] = ZERO;
        _weightOperators[POS_DEC][POS_DEC] = MIN;
        _weightOperators[POS_DEC][NEG_DEC] = MIN;
        _weightOperators[POS_DEC][NEG_INC] = ZERO;

        // Edges outgoing from a NEG_DEC neuron
        _weightOperators[NEG_DEC][POS_INC] = MIN;
        _weightOperators[NEG_DEC][POS_DEC] = ZERO;
        _weightOperators[NEG_DEC][NEG_DEC] = ZERO;
        _weightOperators[NEG_DEC][NEG_INC] = MIN;

        // Edges outgoing from a NEG_INC neuron
        _weightOperators[NEG_INC][POS_INC] = ZERO;
        _weightOperators[NEG_INC][POS_DEC] = MAX;
        _weightOperators[NEG_INC][NEG_DEC] = MAX;
        _weightOperators[NEG_INC][NEG_INC] = ZERO;
    }

    Map<NLR::NeuronIndex, NeuronType> _indexToType;


    void storeBaseQuery( const InputQuery &query )
    {
        _baseQuery = query;
        _baseQuery.constructNetworkLevelReasoner();

        // Store the bounds for the input variables
        NLR::Layer *inputLayer = (NLR::Layer *)_baseQuery.getNetworkLevelReasoner()->getLayer( 0 );
        for ( unsigned i = 0; i < inputLayer->getSize(); ++i )
        {
            unsigned variable = inputLayer->neuronToVariable( i );
            inputLayer->setLb( i, query.getLowerBound( variable ) );
            inputLayer->setUb( i, query.getUpperBound( variable ) );
        }

        _baseQuery.getNetworkLevelReasoner()->dumpTopology();
    }

    void preprocessQuery()
    {
        // Clone the NLR of the base query
        NLR::NetworkLevelReasoner *nlr = new NLR::NetworkLevelReasoner;
        _baseQuery.getNetworkLevelReasoner()->storeIntoOther( *nlr );

        unsigned numberOfLayers = nlr->getNumberOfLayers();

        // All neurons in the output layer are POS_INC, by convention
        // const NLR::Layer *lastLayer = nlr->getLayer( numberOfLayers - 1 );
        // for ( unsigned i = 0; i < lastLayer->getSize(); ++i )
        //     _indexToType[NLR::NeuronIndex( numberOfLayers - 1, i )] = POS_INC;
        preprocessOutputLayer( *nlr, numberOfLayers - 1 );

        // Now, start handling the intermediate layers
        for ( unsigned i = numberOfLayers - 2; i > 0; --i )
        {
            printf( "Preprocessing layer %u\n", i );

            if ( nlr->getLayer( i )->getLayerType() == NLR::Layer::WEIGHTED_SUM )
                preprocessIntermediateWSLayer( *nlr, i );
            else if ( nlr->getLayer( i )->getLayerType() == NLR::Layer::RELU )
                preprocessIntermediateReluLayer( *nlr, i, i == numberOfLayers - 2 );
            else
            {
                printf( "Error! Unsupported layer!\n" );
                exit( 1 );
            }
        }

        // Assign variable indices to all nodes
        unsigned counter = nlr->getLayer( 0 )->getSize();
        for ( unsigned i = 1; i < numberOfLayers; ++i )
        {
            NLR::Layer *layer = (NLR::Layer *)nlr->getLayer( i );
            for ( unsigned j = 0; j < layer->getSize(); ++j )
                layer->setNeuronVariable( j, counter++ );
        }

        printf( "Done preprocessing, now creating the actual query from the NLR\n" );

        _preprocessedQuery = nlr->generateInputQuery();

        // Sanity: use the NLR to evaluate the network
        // double input[5] = { 0.2, 0.2, 0.2, 0.2, 0.2 };
        // double output[5];

        // nlr->evaluate( input, output );

        // printf( "Evaluation result:\n" );
        // for ( unsigned i = 0; i < 5; ++i )
        // {
        //     printf( "\5output[%u] = %.5lf\n", i, output[i] );
        // }

        // _baseQuery.getNetworkLevelReasoner()->evaluate( input, output );

        // printf( "And on the base query:\n" );
        // for ( unsigned i = 0; i < 5; ++i )
        // {
        //     printf( "\5output[%u] = %.5lf\n", i, output[i] );
        // }

        delete nlr;
    }

    void preprocessOutputLayer( NLR::NetworkLevelReasoner &nlr,
                                unsigned layer )
    {
        NLR::Layer *previousLayer = (NLR::Layer *)nlr.getLayer( layer - 1 );
        NLR::Layer *thisLayer = (NLR::Layer *)nlr.getLayer( layer );

        unsigned originalSize = thisLayer->getSize();

        NLR::Layer *preprocessedLayer = new NLR::Layer( thisLayer->getLayerIndex(),
                                                       thisLayer->getLayerType(),
                                                       originalSize,
                                                       &nlr );

        printf( "pp output layer called. Output layer size: %u. Previous layer size: %u\n", originalSize, previousLayer->getSize() );
        preprocessedLayer->addSourceLayer( layer - 1, previousLayer->getSize() * 4 );
        printf( "---\n" );

        for ( unsigned i = 0; i < originalSize; ++i )
        {
            for ( unsigned j = 0; j < previousLayer->getSize(); ++j )
            {
                double weight = thisLayer->getWeight( layer - 1, j, i );

                preprocessedLayer->setWeight( layer - 1, 4 * j    , i, weight );
                preprocessedLayer->setWeight( layer - 1, 4 * j + 1, i, weight );
                preprocessedLayer->setWeight( layer - 1, 4 * j + 2, i, weight );
                preprocessedLayer->setWeight( layer - 1, 4 * j + 3, i, weight );
            }

            // Duplicate the bias
            double bias = thisLayer->getBias( i );
            preprocessedLayer->setBias( i, bias );
        }

        nlr.replaceLayer( layer, preprocessedLayer );
    }

    void preprocessIntermediateReluLayer( NLR::NetworkLevelReasoner &nlr,
                                          unsigned layer,
                                          bool nextLayerIsOutput )
    {
        NLR::Layer *previousLayer = (NLR::Layer *)nlr.getLayer( layer - 1 );
        NLR::Layer *thisLayer = (NLR::Layer *)nlr.getLayer( layer );
        NLR::Layer *nextLayer = (NLR::Layer *)nlr.getLayer( layer + 1 );

        unsigned originalSize = thisLayer->getSize();

        NLR::Layer *preprocessedLayer = new NLR::Layer( thisLayer->getLayerIndex(),
                                                        thisLayer->getLayerType(),
                                                        4 * originalSize,
                                                        &nlr );

        preprocessedLayer->addSourceLayer( layer - 1, previousLayer->getSize() * 4 );

        printf( "PP relu layer. Size: %u. Prev layer size: %u\n", originalSize, previousLayer->getSize() );

        for ( unsigned i = 0; i < originalSize; ++i )
        {
            /*
              the i'th neuron is mapped to neurons 4i .. 4i + 3

              Neuron 4i    : pos neuron, feeding into INC neurons (POS INC)
              Neuron 4i + 1: pos neuron, feeding into DEC neurons (POS DEC)
              Neuron 4i + 2: neg neuron, feeding into INC neurons (NEG DEC)
              Neuron 4i + 3: neg neuron, feeding into DEC neurons (NEG INC)
            */

            // Mark the source neuron for each ReLU
            preprocessedLayer->addActivationSource( layer - 1, 4 * i    , 4 * i );
            preprocessedLayer->addActivationSource( layer - 1, 4 * i + 1, 4 * i + 1 );
            preprocessedLayer->addActivationSource( layer - 1, 4 * i + 2, 4 * i + 2 );
            preprocessedLayer->addActivationSource( layer - 1, 4 * i + 3, 4 * i + 3 );

            // Prune the outgoing edges according to the categories
            for ( unsigned j = 0; j < nextLayer->getSize(); ++j )
            {
                double weight = nextLayer->getWeight( layer, 4 * i, j );
                if ( weight > 0 )
                {
                    // Remove this edge from the NEG neurons
                    nextLayer->removeWeight( layer, 4 * i + 2, j );
                    nextLayer->removeWeight( layer, 4 * i + 3, j );
                }
                else
                {
                    // Remove this edge from the POS neurons
                    nextLayer->removeWeight( layer, 4 * i    , j );
                    nextLayer->removeWeight( layer, 4 * i + 1, j );
                }

                if ( !nextLayerIsOutput )
                {
                    switch ( j % 4 )
                    {
                    case 0:
                    case 3:
                        /*
                          The target is INC. Only maintain edges from
                          <POS,INC> and <NEG,DEC>
                        */
                        nextLayer->removeWeight( layer, 4 * i + 1, j );
                        nextLayer->removeWeight( layer, 4 * i + 3, j );
                        break;

                    case 1:
                    case 2:
                        /*
                          The target is DEC. Only maintain edges from
                          <POS,DEC> and <NEG,INC>
                        */
                        nextLayer->removeWeight( layer, 4 * i    , j );
                        nextLayer->removeWeight( layer, 4 * i + 2, j );
                        break;

                    default:
                        printf( "Unreachable!\n" );
                        exit( 1 );
                    }
                }
                else
                {
                    // The next layer is the output layer, all neurons are pos
                    nextLayer->removeWeight( layer, 4 * i + 1, j );
                    nextLayer->removeWeight( layer, 4 * i + 3, j );
                }
            }
        }

        nlr.replaceLayer( layer, preprocessedLayer );
    }

    void preprocessIntermediateWSLayer( NLR::NetworkLevelReasoner &nlr,
                                        unsigned layer )
    {
        NLR::Layer *previousLayer = (NLR::Layer *)nlr.getLayer( layer - 1 );
        NLR::Layer *thisLayer = (NLR::Layer *)nlr.getLayer( layer );

        unsigned originalSize = thisLayer->getSize();

        NLR::Layer *preprocessedLayer = new NLR::Layer( thisLayer->getLayerIndex(),
                                                        thisLayer->getLayerType(),
                                                        4 * originalSize,
                                                        &nlr );

        preprocessedLayer->addSourceLayer( layer - 1, previousLayer->getSize() * 4 );

        for ( unsigned i = 0; i < originalSize; ++i )
        {
            /*
              the i'th neuron is mapped to neurons 4i .. 4i + 3

              Neuron 4i    : pos neuron, feeding into INC neurons
              Neuron 4i + 1: pos neuron, feeding into DEC neurons
              Neuron 4i + 2: neg neuron, feeding into INC neurons
              Neuron 4i + 3: neg neuron, feeding into DEC neurons
            */

            for ( unsigned j = 0; j < previousLayer->getSize(); ++j )
            {
                double weight = thisLayer->getWeight( layer - 1, j, i );

                preprocessedLayer->setWeight( layer - 1, 4 * j, 4 * i    , weight );
                preprocessedLayer->setWeight( layer - 1, 4 * j, 4 * i + 1, weight );
                preprocessedLayer->setWeight( layer - 1, 4 * j, 4 * i + 2, weight );
                preprocessedLayer->setWeight( layer - 1, 4 * j, 4 * i + 3, weight );

                preprocessedLayer->setWeight( layer - 1, 4 * j + 1, 4 * i    , weight );
                preprocessedLayer->setWeight( layer - 1, 4 * j + 1, 4 * i + 1, weight );
                preprocessedLayer->setWeight( layer - 1, 4 * j + 1, 4 * i + 2, weight );
                preprocessedLayer->setWeight( layer - 1, 4 * j + 1, 4 * i + 3, weight );

                preprocessedLayer->setWeight( layer - 1, 4 * j + 2, 4 * i    , weight );
                preprocessedLayer->setWeight( layer - 1, 4 * j + 2, 4 * i + 1, weight );
                preprocessedLayer->setWeight( layer - 1, 4 * j + 2, 4 * i + 2, weight );
                preprocessedLayer->setWeight( layer - 1, 4 * j + 2, 4 * i + 3, weight );

                preprocessedLayer->setWeight( layer - 1, 4 * j + 3, 4 * i    , weight );
                preprocessedLayer->setWeight( layer - 1, 4 * j + 3, 4 * i + 1, weight );
                preprocessedLayer->setWeight( layer - 1, 4 * j + 3, 4 * i + 2, weight );
                preprocessedLayer->setWeight( layer - 1, 4 * j + 3, 4 * i + 3, weight );
            }

            // Duplicate the bias
            double bias = thisLayer->getBias( i );

            preprocessedLayer->setBias( 4 * i    , bias );
            preprocessedLayer->setBias( 4 * i + 1, bias );
            preprocessedLayer->setBias( 4 * i + 2, bias );
            preprocessedLayer->setBias( 4 * i + 3, bias );
        }

        nlr.replaceLayer( layer, preprocessedLayer );
    }

    void createInitialAbstraction()
    {
        /*
          Begin replacing the layers. Each intermediate layer has 4-tuples of neurons:

              Neuron 4i    : <pos, inc>
              Neuron 4i + 1: <pos, dec>
              Neuron 4i + 2: <neg, dec>
              Neuron 4i + 3: <neg, inc>

          These are transformed into just 4 neurons, in the same order.
        */

        /*
          Assumption: the following layers are not touched:

          1. The input layer
          2. The first hidden layer (WS)
          3. The second hidden layer (ReLU)
          4. The output layer
        */
        const NLR::NetworkLevelReasoner *preprocessedNlr = _preprocessedQuery.getNlr();

        NLR::NetworkLevelReasoner *nlr = new NLR::NetworkLevelReasoner;
        _preprocessedQuery.getNetworkLevelReasoner()->storeIntoOther( *nlr );

        unsigned numberOfLayers = nlr->getNumberOfLayers();

        // Layers 0-2 are copied as-is, with just the layer owner replaced
        for ( unsigned i = 0; i < 2; ++i )
        {
            NLR::Layer *newLayer = new NLR::Layer( preprocessedNlr->getLayer( i ) );
            newLayer->setLayerOwner( nlr );
            nlr->addLayer( i, newLayer );
        }

        // Next, layers 4 - ( numLayers - 1 ) are abstracted to saturation
        for ( unsigned i = 3; i < numberOfLayers - 1; ++i )
        {
            abstractLayerToSaturation( *nlr, i, *preprocessedNlr );
        }
    }

    void abstractLayerToSaturation( NetworkLevelReasoner &nlr, unsigned layer, const NetworkLevelReasoner &preprocessedNlr )
    {
        NLR::Layer *previousLayer = (NLR::Layer *)nlr.getLayer( layer - 1 );
        const NLR::Layer *concretePreviousLayer = preprocessedNlr.getLayer( layer - 1 );
        const NLR::Layer *concreteLayer = preprocessedNlr.getLayer( layer );
        // NLR::Layer *nextLayer = (NLR::Layer *)nlr.getLayer( layer + 1 );

        NLR::Layer::Type type = concreteLayer->getLayerType();

        NLR::Layer *abstractLayer = new NLR::Layer( layer, type, 4, &nlr );
        abstractLayer->addSourceLayer( layer - 1 );

        if ( type == NLR::Layer::WEIGHTED_SUM && layer != 3 )
        {
            /*
              Weights are computed as max-of-sums or min-of-sums
            */

            double min;
            double max;
            unsigned source;
            unsigned target;
            double sum;

            for ( unsigned sourceClass = 0; sourceClass < 4; ++sourceClass )
            {
                for ( unsigned targetClass = 0; targetClass < 4; ++targetClass )
                {
                    if ( _weightOperators[sourceClass][targetClass] == ZERO )
                        continue;

                    max = FloatUtils::negativeInfinity();
                    min = FloatUtils::infinity();

                    source = sourceClass;
                    while ( source < concretePreviousLayer->getSize() )
                    {
                        sum = 0;
                        target = targetClass;
                        while ( target < concreteLayer->getSize() )
                        {
                            sum += concreteLayer->getWeight( layer - 1, source, target );
                            target += 4;
                        }
                    }

                    if ( _weightOperators[sourceClass][targetClass] == MAX )
                        max = FloatUtils::max( max, sum );
                    else
                        min = FloatUtils::min( min, sum );

                    source += 4;
                }

                if ( _weightOperators[sourceClass][targetClass] == MAX )
                    abstractLayer->setWeight( layer - 1,
                                              sourceClass,
                                              targetClass,
                                              max );
                else ( _weightOperators[sourceClass][targetClass] == MIN )
                    abstractLayer->setWeight( layer - 1,
                                              sourceClass,
                                              targetClass,
                                              min );
            }
        }

        else if ( type == NLR::Layer::WEIGHTED_SUM && layer == 3 )
        {
            // Special case, the first weighted sum layer that is processed
            // No sums, just mins/maxes

            double min;
            double max;
            unsigned source;
            unsigned target;
            double sum;

            for ( unsigned sourceClass = 0; sourceClass < 4; ++sourceClass )
            {
                for ( unsigned targetClass = 0; targetClass < 4; ++targetClass )
                {
                    if ( _weightOperators[sourceClass][targetClass] == ZERO )
                        continue;


                    max = FloatUtils::negativeInfinity();
                    min = FloatUtils::infinity();

                    source = sourceClass;
                    while ( source < concretePreviousLayer->getSize() )
                    {
                        target = targetClass;
                        while ( target < concreteLayer->getSize() )
                        {
                            if ( _weightOperators[sourceClass][targetClass] == MAX )
                                max = FloatUtils::max( max,
                                                       concreteLayer->getWeight( layer - 1, source, target ) );
                            else
                                min = FloatUtils::min( min,
                                                       concreteLayer->getWeight( layer - 1, source, target ) );

                            target += 4;
                        }
                    }

                    if ( _weightOperators[sourceClass][targetClass] == MAX )
                        abstractLayer->setWeight( layer - 1, source, target, max );
                    else
                        abstractLayer->setWeight( layer - 1, source, target, min );

                    source += 4;
                }
            }
        }

        /*
          TODOs:
          - double check, especially for first set of cases, the order of sum/max
          - Handle output layer?
          - biases
          - check...
        */

        else if ( type == NLR::Layer::RELU )
        {
            // Map neuron i to neuron i...
            for ( unsigned i = 0; i < 4; ++i )
                abstractLayer->addActivationSource( layer - 1, i, i );
        }
        else
        {
            printf( "Error, layer type %u not supported!\n", type );
            exit( 1 );
        }

        nlr->addLayer( layer, abstractLayer );
    }

    void solve()
    {
        Engine engine;

        if ( engine.processInputQuery( _currentQuery ) )
            engine.solve( Options::get()->getInt( Options::TIMEOUT ) );

        _engineExitCode = engine.getExitCode();

        if ( _engineExitCode != Engine::SAT &&
             _engineExitCode != Engine::UNSAT )
        {
            printf( "Error! Unsupported return code by engine\n" );
            exit( 1 );
        }

        if ( _engineExitCode == Engine::SAT )
            engine.extractSolution( _currentQuery );
    }

    bool sat()
    {
        return _engineExitCode == Engine::SAT;
    }

    bool spurious()
    {
        return false;
    }

    bool unsat()
    {
        return _engineExitCode == Engine::UNSAT;
    }

    void refine()
    {
    }
};

#endif // __CEGARSolver_h__
