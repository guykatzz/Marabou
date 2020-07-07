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
        exit( 1 );

        createInitialAbstraction();

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
    InputQuery _baseQuery;
    InputQuery _currentQuery;
    Engine::ExitCode _engineExitCode;

    enum NeuronType {
        POS_INC = 0,
        POS_DEC = 1,
        NEG_INC = 2,
        NEG_DEC = 3,
    };

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

        _currentQuery = nlr->generateInputQuery();
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

        printf( "OLD output layer address is: %p\n", thisLayer );
        printf( "NEW output layer address is: %p\n", preprocessedLayer );

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
            preprocessedLayer->addActivationSource( layer - 1, 4 * i + 1, 4 * i );
            preprocessedLayer->addActivationSource( layer - 1, 4 * i + 2, 4 * i );
            preprocessedLayer->addActivationSource( layer - 1, 4 * i + 3, 4 * i );

            // Prune the outgoing edges according to the categories
            for ( unsigned j = 0; j < nextLayer->getSize(); ++j )
            {
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
                    printf( "Next layer is output, removing weights...\n" );
                    // The next layer is the output layer, all neurons are pos
                    printf( "Output layer address is: %p\n", nextLayer );
                    nextLayer->removeWeight( layer, 4 * i + 1, j );
                    nextLayer->removeWeight( layer, 4 * i + 3, j );
                    printf( "Next layer is output, removing weights... DONE\n" );
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
        //        _currentQuery = _baseQuery;
        _currentQuery = _baseQuery.getNetworkLevelReasoner()->generateInputQuery();

        printf( "Dumping the base query...\n" );
        _baseQuery.dump();

        printf( "\n\n*********\n\n" );

        printf( "Dumping the restored query...\n" );
        _currentQuery.dump();

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
