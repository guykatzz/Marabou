/*********************                                                        */
/*! \file PiecewiseLinearConstraint.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __PiecewiseLinearConstraint_h__
#define __PiecewiseLinearConstraint_h__

#include "ITableau.h"
#include "Map.h"
#include "PiecewiseLinearCaseSplit.h"
#include "Queue.h"

class ITableau;

class PiecewiseLinearConstraintStateData
{
public:
    virtual ~PiecewiseLinearConstraintStateData() {}
};

// TODO: Why do we need 2 separate levels? Why not just "PiecewiseLinearConstraintState"?
class PiecewiseLinearConstraintState
{
    /*
      PL constraint saved states include the following:
      - enqueued splits (tightenings/equations)
      - any additional data that the specific constraint may want to save
    */
public:
    PiecewiseLinearConstraintState( )
        : _stateData( NULL )
    {
    }

    ~PiecewiseLinearConstraintState()
    {
        if ( _stateData )
        {
            delete _stateData;
            _stateData = NULL;
        }
    }

    Queue<PiecewiseLinearCaseSplit> _splits;
    PiecewiseLinearConstraintStateData *_stateData;
};

class PiecewiseLinearConstraint : public ITableau::VariableWatcher
{
public:
    /*
      A possible fix for a violated piecewise linear constraint: a
      variable whose value should be changed.
    */
    struct Fix
    {
    public:
        Fix( unsigned variable, double value )
            : _variable( variable )
            , _value( value )
        {
        }

        unsigned _variable;
        double _value;
    };

    virtual ~PiecewiseLinearConstraint() {}

    /*
      Register/unregister the constraint with a talbeau.
    */
    virtual void registerAsWatcher( ITableau *tableau ) = 0;
    virtual void unregisterAsWatcher( ITableau *tableau ) = 0;

    /*
      The variable watcher notifcation callbacks.
    */
    virtual void notifyVariableValue( unsigned /* variable */, double /* value */ ) {}
    virtual void notifyLowerBound( unsigned /* variable */, double /* bound */ ) {}
    virtual void notifyUpperBound( unsigned /* variable */, double /* bound */ ) {}

    /*
      Turn the constraint on/off.
    */
    virtual void setActiveConstraint( bool active ) = 0;
    virtual bool isActive() const = 0;

    /*
      Returns true iff the variable participates in this piecewise
      linear constraint.
    */
    virtual bool participatingVariable( unsigned variable ) const = 0;

    /*
      Get the list of variables participating in this constraint.
    */
    virtual List<unsigned> getParticiatingVariables() const = 0;

    /*
      Returns true iff the assignment satisfies the constraint.
    */
    virtual bool satisfied() const = 0;

    /*
      Returns a list of possible fixes for the violated constraint.
    */
    virtual List<PiecewiseLinearConstraint::Fix> getPossibleFixes() const = 0;

    /*
      Returns the list of case splits that this piecewise linear
      constraint breaks into. These splits need to complementary,
      i.e. if the list is {l1, l2, ..., ln-1, ln},
      then ~l1 /\ ~l2 /\ ... /\ ~ln-1 --> ln.
    */
    virtual List<PiecewiseLinearCaseSplit> getCaseSplits() const = 0;

    /*
      Check if the constraint's phase has been fixed.
    */
    virtual bool phaseFixed() const = 0;

    /*
      If the constraint's phase has been fixed, get the (valid) case split.
    */
    virtual PiecewiseLinearCaseSplit getValidCaseSplit() const = 0;

    /*
      Store and restore the constraint's state. Needed for case splitting
      and backtracking. The stored elements are the fields
      specified in the PiecewiseLinearConstraintState
    */
    virtual void storeState( PiecewiseLinearConstraintState &state ) const = 0;
    virtual void restoreState( const PiecewiseLinearConstraintState &state ) = 0;

protected:
    Queue<PiecewiseLinearCaseSplit> _splits;
};

#endif // __PiecewiseLinearConstraint_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//