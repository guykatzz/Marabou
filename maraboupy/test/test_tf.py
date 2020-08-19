# Supress warnings caused by tensorflow
import warnings
warnings.filterwarnings('ignore', category = DeprecationWarning)
warnings.filterwarnings('ignore', category = PendingDeprecationWarning)

import pytest
from .. import Marabou
import numpy as np
import os

# Global settings
OPT = Marabou.createOptions(verbosity = 0)        # Turn off printing
TOL = 1e-4                                        # Set tolerance for checking Marabou evaluations
FG_FOLDER = "../../resources/tf/frozen_graph/"    # Folder for test networks written in frozen graph format
SM1_FOLDER = "../../resources/tf/saved_model_v1/" # Folder for test networks written in SavedModel format from tensorflow v1.X
SM2_FOLDER = "../../resources/tf/saved_model_v2/" # Folder for test networks written in SavedModel format from tensorflow v2.X
np.random.seed(123)                               # Seed random numbers for repeatability
NUM_RAND = 5                                      # Default number of random test points per example

def test_fc1():
    """
    Test a fully-connected neural network
    Uses Const, Identity, Placeholder, MutMul, Add, and Relu layers
    """
    filename = os.path.join(os.path.dirname(__file__), FG_FOLDER, "fc1.pb")
    network = Marabou.read_tf(filename)
    evaluateNetwork(network)

def test_fc2():
    """
    Test a fully-connected neural network.
    This network tests different types of Mul and RealDiv layers.
    """
    filename = os.path.join(os.path.dirname(__file__), FG_FOLDER, "fc2.pb")
    network = Marabou.read_tf(filename, outputName = "y")
    evaluateNetwork(network)

def test_KJ_TinyTaxiNet():
    """
    Test a convolutional network without max-pooling
    Uses Const, Identity, Placeholder, Conv2D, BiasAdd, Reshape, 
    MatMul, Add, and Relu layers
    """
    filename = os.path.join(os.path.dirname(__file__), FG_FOLDER, "KJ_TinyTaxiNet.pb")
    network = Marabou.read_tf(filename)
    evaluateNetwork(network)

def test_conv_mp1():
    """
    Test a convolutional network using max pool
    Uses Const, Identity, Placeholder, Conv2D, Add, Relu, and MaxPool layers
    """
    filename = os.path.join(os.path.dirname(__file__), FG_FOLDER, "conv_mp1.pb")
    network = Marabou.read_tf(filename)
    evaluateNetwork(network)

def test_conv_mp2():
    """
    Test a convolutional network using max pool
    This network tests different padding types for convolutional layers
    """
    filename = os.path.join(os.path.dirname(__file__), FG_FOLDER, "conv_mp2.pb")
    network = Marabou.read_tf(filename)
    evaluateNetwork(network)

def test_conv_mp3():
    """
    Test a convolutional network using max pool
    This network tests SAME padding for max pool, as well as a convolutional network
    where the first dimension is an integer value rather than None
    """
    filename = os.path.join(os.path.dirname(__file__), FG_FOLDER, "conv_mp3.pb")
    network = Marabou.read_tf(filename)
    evaluateNetwork(network)

def test_conv_NCHW():
    """
    Test a convolutional network using max pool
    These networks are identical, except they use different data format conventions.
    NHWC means tensors are written in the [Num, Height, Width, Channels] convention.
    NCHW means tensors are written in the [Num, Channels, Height, Width] convention.
    Only the default, NHWC, can be run in tensorflow on CPUs, so this tests the GPU NCHW
    convention by comparing the outputs generated by Marabou to the NHWC outupt from tensorflow
    """
    filename = os.path.join(os.path.dirname(__file__), FG_FOLDER, "conv_mp4.pb")
    network_nhwc = Marabou.read_tf(filename)
    filename = os.path.join(os.path.dirname(__file__), FG_FOLDER, "conv_mp5.pb")
    network_nchw = Marabou.read_tf(filename)

    # Evaluate test points using both Marabou and Tensorflow, and assert that the max error is less than TOL
    testInputs = [[np.random.random(inVars.shape) for inVars in network_nhwc.inputVars] for _ in range(5)]
    for testInput in testInputs:
        mar_nhwc = network_nhwc.evaluateWithMarabou(testInput, options = OPT, filename = "")
        mar_nchw = network_nchw.evaluateWithMarabou(testInput, options = OPT, filename = "")
        tf_nhwc = network_nhwc.evaluateWithoutMarabou(testInput)

        assert max(abs(mar_nhwc - tf_nhwc).flatten()) < TOL
        assert max(abs(mar_nchw - tf_nhwc).flatten()) < TOL

def test_sm1_fc1():
    """
    Test a fully-connected neural network, written in the 
    SavedModel format created by tensorflow version 1.X
    """
    filename = os.path.join(os.path.dirname(__file__), SM1_FOLDER, "fc1")
    network = Marabou.read_tf(filename, modelType = "savedModel_v1", outputName = "add_3")
    evaluateNetwork(network)

def test_sm2_fc1():
    """
    Test a fully-connected neural network, written in the 
    SavedModel format created by tensorflow version 2.X
    """
    filename = os.path.join(os.path.dirname(__file__), SM2_FOLDER, "fc1")
    network = Marabou.read_tf(filename, modelType = "savedModel_v2")
    evaluateNetwork(network)

def test_sm2_sign():
    """
    Test a fully-connected neural network with sign activations, written in the
    SavedModel format created by tensorflow version 2.X
    """
    filename = os.path.join(os.path.dirname(__file__), SM2_FOLDER, "signNetwork")
    network = Marabou.read_tf(filename, modelType = "savedModel_v2")
    evaluateNetwork(network)

def test_sub_concat():
    """
    Test a fully-connected neural network
    This network has two inputs (X0 and X1) that undergo independent operations before being concatenated
    together (concat). The inputs are used again via addition and subtraction to create the output, Y.
    This function tests different configurations of input and output variables, as well as a network where
    the first dimension is an integer rather than None.
    """    
    filename = os.path.join(os.path.dirname(__file__), FG_FOLDER, "sub_concat.pb")

    # Test default, which should find both X0 and X1 for inputs
    network = Marabou.read_tf(filename)
    evaluateNetwork(network)
    assert len(network.inputVars) == 2
    assert network.outputVars.shape == (5,2)

    # If an intermediate layer is used as the output, which depends on only one input variable, 
    # then only that input variable is used
    network = Marabou.read_tf(filename, outputName = "Relu_2")
    assert len(network.inputVars) == 1
    # All output variables come from a ReLU activation, so they should be a part of a PL constraint,
    # and they should have a lower bound
    assert np.all([network.participatesInPLConstraint(var) for var in network.outputVars.flatten()])
    assert np.all([network.lowerBoundExists(var) for var in network.outputVars.flatten()])
    evaluateNetwork(network)
    # Evaluation does not add permanent upper/lower bound values to the network
    for inputVars in network.inputVars:
        assert not np.any([network.lowerBoundExists(var) for var in inputVars.flatten()])
        assert not np.any([network.upperBoundExists(var) for var in inputVars.flatten()])

    # Test that the output of a MatMul operation can be used as output operation
    network = Marabou.read_tf(filename, inputNames = ["X0"], outputName = "MatMul_2")
    evaluateNetwork(network)
    assert len(network.outputVars[1]) == 20

    # Test that concatenation can be defined as output operation
    network = Marabou.read_tf(filename, inputNames = ["X0", "X1"], outputName = "concat")
    evaluateNetwork(network)
    assert len(network.outputVars[1]) == 40

    # An intermediate layer can be used as an input, which forces that layer to have the given values
    # and ignore the equations used to create that intermediate layer
    network = Marabou.read_tf(filename, inputNames = ["X0","X1","concat"], outputName = "Y")
    evaluateNetwork(network)

def test_sub_matmul():
    """
    Test a fully-connected neural network
    This network tests a variety of ways that matmul and subtraction can be used
    """    
    filename = os.path.join(os.path.dirname(__file__), FG_FOLDER, "sub_matmul.pb")
    network = Marabou.read_tf(filename)
    evaluateNetwork(network)

def test_errors():
    """
    Test that user errors of the parser can be caught and helpful information is given
    """
    filename = os.path.join(os.path.dirname(__file__), FG_FOLDER, "sub_concat.pb")
    # Bad modelType
    with pytest.raises(RuntimeError, match=r"Unknown input to modelType"):
        network = Marabou.read_tf(filename, modelType="badModelType")

    # Input name not found in graph
    with pytest.raises(RuntimeError, match=r"input.*is not an operation"):
        network = Marabou.read_tf(filename, inputNames = ["X123"], outputName = "MatMul_2")

    # Output name not found in graph
    with pytest.raises(RuntimeError, match=r"output.*is not an operation"):
        network = Marabou.read_tf(filename, inputNames = ["X0"], outputName = "MatMul_123")

    # Output also used as an input
    with pytest.raises(RuntimeError, match=r"cannot be used as both input and output"):
        network = Marabou.read_tf(filename, inputNames = ["Relu"], outputName = "Relu")

    # One of the inputs is not needed to compute the output
    with pytest.raises(RuntimeError, match=r"not all inputs contributed to the output"):
        network = Marabou.read_tf(filename, inputNames = ["X0","X1"], outputName = "MatMul_2")

    # There are missing inputs, which are needed to compute the output
    with pytest.raises(RuntimeError, match=r"output.*depends on placeholder"):
        network = Marabou.read_tf(filename, inputNames = ["concat"], outputName = "Relu")

    # Not enough input values are given for all input variables
    with pytest.raises(RuntimeError, match=r"Bad input given"):
        network = Marabou.read_tf(filename, inputNames = ["X0","X1"], outputName = "Y")
        testInput = [np.random.random(inVars.shape) for inVars in network.inputVars]
        testInput = testInput[1:]
        network.evaluateWithoutMarabou(testInput)

    # Input values given have the wrong shape, and they cannot be reshaped to the correct shape
    with pytest.raises(RuntimeError, match=r"Input.*should have shape"):
        network = Marabou.read_tf(filename, inputNames = ["X0","X1"], outputName = "Y")
        testInput = [np.random.random((2,) + inVars.shape) for inVars in network.inputVars]
        network.evaluateWithoutMarabou(testInput)

def evaluateNetwork(network, testInputs = None, numPoints = NUM_RAND):
    """
    Evaluate a network at random testInputs with and without Marabou
    Args:
        network (MarabouNetwork): network loaded into Marabou to be evaluated
    """    
    # Create test points if none provided. This creates a list of test points.
    # Each test point is itself a list, representing the values for each input array.
    if not testInputs:
        testInputs = [[np.random.random(inVars.shape) for inVars in network.inputVars] for _ in range(numPoints)]

    # Evaluate test points using both Marabou and Tensorflow, and assert that the max error is less than TOL
    for testInput in testInputs:
        assert max(network.findError(testInput, options = OPT, filename = "").flatten()) < TOL
