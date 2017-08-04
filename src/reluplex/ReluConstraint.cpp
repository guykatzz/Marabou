/*********************                                                        */
/*! \file ReluConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "Debug.h"
#include "FloatUtils.h"
#include "FreshVariables.h"
#include "ITableau.h"
#include "PiecewiseLinearCaseSplit.h"
#include "ReluConstraint.h"
#include "ReluplexError.h"

ReluConstraint::ReluConstraint( unsigned b, unsigned f )
    : _constraintActive( true )
    , _b( b )
    , _f( f )
    , _phaseStatus( PhaseStatus::PHASE_NOT_FIXED )
{
}

void ReluConstraint::registerAsWatcher( ITableau *tableau )
{
    tableau->registerToWatchVariable( this, _b );
    tableau->registerToWatchVariable( this, _f );
}

void ReluConstraint::unregisterAsWatcher( ITableau *tableau )
{
    tableau->unregisterToWatchVariable( this, _b );
    tableau->unregisterToWatchVariable( this, _f );
}

void ReluConstraint::setActiveConstraint( bool active )
{
    _constraintActive = active;
}

bool ReluConstraint::isActive() const
{
    return _constraintActive;
}

void ReluConstraint::notifyVariableValue( unsigned variable, double value )
{
    _assignment[variable] = value;
}

void ReluConstraint::notifyLowerBound( unsigned variable, double bound )
{
    if ( variable == _f && FloatUtils::isPositive( bound ) )
        _phaseStatus = PhaseStatus::PHASE_ACTIVE;
    else if ( variable == _b && !FloatUtils::isNegative( bound ) )
        _phaseStatus = PhaseStatus::PHASE_ACTIVE;
}

void ReluConstraint::notifyUpperBound( unsigned variable, double bound )
{
    if ( ( variable == _f || variable == _b ) && !FloatUtils::isPositive( bound ) )
        _phaseStatus = PhaseStatus::PHASE_INACTIVE;
}

bool ReluConstraint::participatingVariable( unsigned variable ) const
{
    return ( variable == _b ) || ( variable == _f );
}

List<unsigned> ReluConstraint::getParticiatingVariables() const
{
    return List<unsigned>( { _b, _f } );
}

bool ReluConstraint::satisfied() const
{
    if ( !( _assignment.exists( _b ) && _assignment.exists( _f ) ) )
        throw ReluplexError( ReluplexError::PARTICIPATING_VARIABLES_ABSENT );

    double bValue = _assignment.get( _b );
    double fValue = _assignment.get( _f );

    ASSERT( !FloatUtils::isNegative( fValue ) );

    if ( FloatUtils::isPositive( fValue ) )
        return FloatUtils::areEqual( bValue, fValue );
    else
        return !FloatUtils::isPositive( bValue );
}

List<PiecewiseLinearConstraint::Fix> ReluConstraint::getPossibleFixes() const
{
    ASSERT( !satisfied() );
    ASSERT( _assignment.exists( _b ) );
    ASSERT( _assignment.exists( _f ) );

    double bValue = _assignment.get( _b );
    double fValue = _assignment.get( _f );

    ASSERT( !FloatUtils::isNegative( fValue ) );

    List<PiecewiseLinearConstraint::Fix> fixes;

    // Possible violations:
    //   1. f is positive, b is positive, b and f are disequal
    //   2. f is positive, b is non-positive
    //   3. f is zero, b is positive
    if ( FloatUtils::isPositive( fValue ) )
    {
        if ( FloatUtils::isPositive( bValue ) )
        {
            fixes.append( PiecewiseLinearConstraint::Fix( _b, fValue ) );
            fixes.append( PiecewiseLinearConstraint::Fix( _f, bValue ) );
        }
        else
        {
            fixes.append( PiecewiseLinearConstraint::Fix( _b, fValue ) );
            fixes.append( PiecewiseLinearConstraint::Fix( _f, 0 ) );
        }
    }
    else
    {
        fixes.append( PiecewiseLinearConstraint::Fix( _b, 0 ) );
        fixes.append( PiecewiseLinearConstraint::Fix( _f, bValue ) );
    }

    return fixes;
}

List<PiecewiseLinearCaseSplit> ReluConstraint::getCaseSplits() const
{
    if ( _phaseStatus != PhaseStatus::PHASE_NOT_FIXED )
        throw ReluplexError( ReluplexError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

    // Auxiliary variable bound, needed for either phase
    List<PiecewiseLinearCaseSplit> splits;
    unsigned auxVariable = FreshVariables::getNextVariable();

    splits.append( getActiveSplit( auxVariable ) );
    splits.append( getInactiveSplit( auxVariable ) );

    return splits;
}

void ReluConstraint::storeState( PiecewiseLinearConstraintState &state ) const
{
    state._stateData = new ReluConstraintStateData;
    state._splits = _splits;
    ReluConstraintStateData *stateData =
        dynamic_cast<ReluConstraintStateData *>(state._stateData);
    stateData->_constraintActive = _constraintActive;
    stateData->_assignment = _assignment;
    stateData->_phaseStatus = _phaseStatus;
}

void ReluConstraint::restoreState( const PiecewiseLinearConstraintState &state )
{
    _splits = state._splits;
    const ReluConstraintStateData *stateData =
        dynamic_cast<const ReluConstraintStateData *>(state._stateData);
    ASSERT( stateData );
    _constraintActive = stateData->_constraintActive;
    _assignment = stateData->_assignment;
    _phaseStatus = stateData->_phaseStatus;
}

PiecewiseLinearCaseSplit ReluConstraint::getInactiveSplit( unsigned auxVariable ) const
{
    // Inactive phase: b <= 0, f = 0
    PiecewiseLinearCaseSplit inactivePhase;
    Tightening inactiveBound( _b, 0.0, Tightening::UB );
    inactivePhase.storeBoundTightening( inactiveBound );
    Equation inactiveEquation;
    inactiveEquation.addAddend( 1, _f );
    inactiveEquation.addAddend( 1, auxVariable );
    inactiveEquation.markAuxiliaryVariable( auxVariable );
    inactiveEquation.setScalar( 0 );
    inactivePhase.addEquation( inactiveEquation );
    Tightening auxUpperBound( auxVariable, 0.0, Tightening::UB );
    Tightening auxLowerBound( auxVariable, 0.0, Tightening::LB );
    inactivePhase.storeBoundTightening( auxUpperBound );
    inactivePhase.storeBoundTightening( auxLowerBound );
    return inactivePhase;
}

PiecewiseLinearCaseSplit ReluConstraint::getActiveSplit( unsigned auxVariable ) const
{
    // Active phase: b >= 0, b - f = 0
    PiecewiseLinearCaseSplit activePhase;
    Tightening activeBound( _b, 0.0, Tightening::LB );
    activePhase.storeBoundTightening( activeBound );
    Equation activeEquation;
    activeEquation.addAddend( 1, _b );
    activeEquation.addAddend( -1, _f );
    activeEquation.addAddend( 1, auxVariable );
    activeEquation.markAuxiliaryVariable( auxVariable );
    activeEquation.setScalar( 0 );
    activePhase.addEquation( activeEquation );
    Tightening auxUpperBound( auxVariable, 0.0, Tightening::UB );
    Tightening auxLowerBound( auxVariable, 0.0, Tightening::LB );
    activePhase.storeBoundTightening( auxUpperBound );
    activePhase.storeBoundTightening( auxLowerBound );
    return activePhase;
}

bool ReluConstraint::phaseFixed() const
{
    return _phaseStatus != PhaseStatus::PHASE_NOT_FIXED;
}

PiecewiseLinearCaseSplit ReluConstraint::getValidCaseSplit() const
{
    ASSERT( _phaseStatus != PhaseStatus::PHASE_NOT_FIXED );

    unsigned auxVariable = FreshVariables::getNextVariable();

    if ( _phaseStatus == PhaseStatus::PHASE_ACTIVE )
        return getActiveSplit( auxVariable );

    return getInactiveSplit( auxVariable );
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//