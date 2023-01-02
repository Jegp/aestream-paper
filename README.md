# AEStream-paper

This repository contains instructions as well as code to reproduce the findings in the paper [AEStream: Accelerated event-based processing with coroutines](https://arxiv.org/abs/2212.10719).

## Installation instructions

We assume [AEStream](https://github.com/norse/aestream) to be installed at > v.0.4.
Additionally, we require CMake and a C++20 compiler with support for coroutines.

## Reproducing C++ benchmarks

```bash
$ mkdir build
$ cd build
$ cmake -Gninja ..
$ ./aestream-paper
```

## Reproducing GPU benchmarks
Install the GPU version of PyTorch along with  [the spiking neural network simulator Norse](https://github.com/norse/norse).
Then run:

```bash
$ python3 src/gpu_benchmark.py
```

## Reproducing plots

1. Start a [Jupyter Notebook server](https://jupyter.org/)
2. Execute the `plots.ipynb`

## Acknowledgements

This work was done by [Jens Egholm Pedersen](https://jepedersen.dk) at [KTH](https://kth.se).

We foremost would like to thank Anders Bo Sørensen for his friendly and invaluable help with CUDA and GPU profiling. Emil Jansson deserves our gratitude for scrutinizing and improving the coroutine benchmark C++ code. We gracefully recognize funding from the EC Horizon 2020 Framework Programme under Grant Agreements 785907 and 945539 (HBP). Our thanks also extend to the Pioneer Centre for AI, under the Danish National Research Foundation grant number P1, for hosting us. 