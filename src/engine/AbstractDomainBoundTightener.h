/*********************                                                        */
/*! \file AbstractDomainBoundTightener.h
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

#ifndef __AbstractDomainBoundTightener_h__
#define __AbstractDomainBoundTightener_h__

#include "NeuronIndex.h"
#include "PiecewiseLinearFunctionType.h"

/*
  A superclass for performing abstract-interpretation-based bound
  tightening. This is a virtual class: a child class must provide the
  specific of the actual abstract domain that is being used (e.g.,
  box, polyhedron).
*/

class AbstractDomainBoundTightener
{
public:
    AbstractDomainBoundTightener();
    ~AbstractDomainBoundTightener();

    void initialize( unsigned numberOfLayers,
                     const Map<unsigned, unsigned> *layerSizes,
                     const Map<NeuronIndex, PiecewiseLinearFunctionType> *neuronToActivationFunction,
                     const double **weights,
                     const Map<NeuronIndex, double> *bias )
    {
        _numberOfLayers = numberOfLayers;
        _layerSizes = layerSizes;
        _neuronToActivationFunction = neuronToActivationFunction;
        _weights = weights;
        _bias = bias;
    }

    void run()
    {
        // Step 1: grab the input constraints
        createInputAbstractValue();

        // Step 2: propagate through the hidden layers

        for ( _currentLayer = 1; _currentLayer < _numberOfLayers; ++_currentLayer )
        {
            // Apply the weighted sum
            performAffineTransformation();

            // Apply the activation function
            applyActivationFunction();
        }

    }

private:
    unsigned _numberOfLayers;
    const Map<unsigned, unsigned> *_layerSizes;
    const Map<NeuronIndex, PiecewiseLinearFunctionType> *_neuronToActivationFunction;
    const double **_weights;
    const Map<NeuronIndex, double> *_bias;

    unsigned _currentLayer;

    void createInputAbstractValue()
    {
    }

    void performAffineTransformation()
    {
    }

    void applyActivationFunction()
    {
    }
};

#endif // __AbstractDomainBoundTightener_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
