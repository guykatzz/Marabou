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
        const NLR::Layer *lastLayer = nlr->getLayer( numberOfLayers - 1 );
        for ( unsigned i = 0; i < lastLayer->getSize(); ++i )
            _indexToType[NLR::NeuronIndex( numberOfLayers - 1, i )] = POS_INC;

        // Now, start handling the intermediate layers
        for ( unsigned i = numberOfLayers - 2; i > 0; --i )
            preprocessIntermediateLayer( *nlr, i );

        delete nlr;
    }

    void preprocessIntermediateLayer( NLR::NetworkLevelReasoner &nlr,
                                      unsigned layer )
    {
        NLR::Layer *previousLayer = (NLR::Layer *)nlr.getLayer( layer - 1 );
        NLR::Layer *thisLayer = (NLR::Layer *)nlr.getLayer( layer );
        NLR::Layer *nextLayer = (NLR::Layer *)nlr.getLayer( layer + 1 );

        unsigned originalSize = thisLayer->getSize();

        NLR::Layer preprocessedLayer = new NLR::Layer( thisLayer->getLayerIndex(),
                                                       thisLayer->getLayerType(),
                                                       4 * originalSize,
                                                       &nlr );

        for ( unsigned i = 0; i < originalSize; ++i )
        {
            // the i'th neuron is mapped to neurons 4i .. 4i + 3
        }

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
