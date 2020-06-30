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

#include "InputQuery.h"

class CEGARSolver
{
public:

    void run( InputQuery &query )
    {
        storeBaseQuery( query );

        preprocessQuery();

        createInitialAbstraction();

        while ( true )
        {
            solve();

            if ( sat() && !spurious() )
            {
                // Truly SAT
                return;
            }
            else if ( unsat() )
            {
                // Truly UNSAT
                return;
            }

            refine();
        }
    }

private:
    InputQuery _baseQuery;

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
    }

    void solve()
    {
    }

    bool sat()
    {
        return false;
    }

    bool spurious()
    {
        return true;
    }

    bool unsat()
    {
        return true;
    }

    void refine()
    {
    }
};

#endif // __CEGARSolver_h__
