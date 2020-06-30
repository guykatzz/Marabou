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

    void storeBaseQuery( const InputQuery &query )
    {
        _baseQuery = query;
        _baseQuery.constructNetworkLevelReasoner();
        _baseQuery.getNetworkLevelReasoner()->dumpTopology();
    }

    void preprocessQuery()
    {
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
