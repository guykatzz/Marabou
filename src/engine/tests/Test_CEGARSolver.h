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

    void test_dummy()
    {
        TS_TRACE( 1 );
    }
};
