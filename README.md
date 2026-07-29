# QRAT: A Probabilistic Reachability Analysis Tool for Quantum Programs
This repository presents a probabilistic reachability analysis tool called `QRAT` for quantum programs. The tool is written in C++ using alternative backends [MQT Core](https://github.com/cda-tum/mqt-core) and [exact-dd](https://github.com/canhminhdo/exact-dd) to simulate quantum computation.

## Simulation backends
QRAT supports two alternative backends for simulating quantum computation, selected with the `set backend` command:

- `set backend mqt .` (default) - MQT Core's floating-point decision diagram package. Supports the full gate set, including parametrized rotations.
- `set backend dw .` - the exact decision diagram package from [exact-dd](https://github.com/canhminhdo/exact-dd), whose amplitudes are exact elements of Q[ω] (ω = e^{iπ/4}). Only the Clifford+T gate set is supported (`I, X, Y, Z, H, S, Sdg, T, Tdg, V, Vdg, SX, SXdg, SWAP, iSWAP, iSWAPdg, DCX` and their controlled variants).

## Dependencies
- [MQT Core](https://github.com/cda-tum/mqt-core) - The Backbone of the Munich Quantum Toolkit
- [exact-dd](https://github.com/canhminhdo/exact-dd) - Exact decision diagram package for quantum computation
- [C++ Compilier](https://clang.llvm.org) - Supporting C++17
- [CMake](https://cmake.org) - A minimum version of 3.19
- [google/googletest](https://github.com/google/googletest) - A testing framework for C++ (only used in tests)
- [Flex](https://github.com/westes/flex) - A version of 2.6.4
- [Bison](https://www.gnu.org/software/bison/) - A version of 3.8.2

## Installation
`QRAT` can be installed on your system using several ways as follows:

### 1. Downloading from executable files
QRAT executable files are available for macOS and Linux at the following link.
```
https://github.com/canhminhdo/qrat/releases/tag/v2.0
```

### 2. Running QRAT from Docker

The `qrat` program is available from the command line in the Docker container.

```shell
$ docker build --no-cache -t qrat .
$ docker run --rm -it qrat
```

### 3. Building from source
`QRAT` can be installed on your system after building it from the source.
```shell
$ git clone --recurse-submodules https://github.com/canhminhdo/qrat
$ cd qrat
$ cmake -S . -B build -D CMAKE_BUILD_TYPE=Release # configure the project to be built with `Release` mode
$ cmake --build build --config Release -j 8 # building the project with `Release` mode and 8 workers in parallel
$ ctest --test-dir build # testing the project with some test cases
$ cmake --install build --config Release # install the project with `Release` mode in the `install` folder
$ cpack -G "ZIP" --config build/CPackConfig.cmake -B package # package the project to be distributed in the `package` directory
```

## Getting Started
We can use QRAT to verify the correctness of Quantum Teleportation easily as follows.

- Go to the `examples/teleport` folder
```
$ cd examples/teleport
```

- Check the `teleport.qw` program file whose content is as follows
```
prog TELEPORT is
var q0, q1, q2 : qubit;
init
    q0 := random;
    q1 := |0>;
    q2 := |0>;
begin
    H[q1];
    CX[q1, q2];
    CX[q0, q1];
    H[q0];
    if M[q1] = 1 then X[q2]; else skip; fi;
    if M[q0] = 1 then Z[q2]; else skip; fi;
end

// for qualitative reachability analysis
search in TELEPORT with =>! such that P(q2, init[q0]) .

// for quantitative reachability analysis
psearch in TELEPORT with =>! such that P(q2, init[q0]) .
```

- Feed the `teleport.qw` program file into `QRAT` to verify its correctness.
```shell
qrat teleport.qw
```

## Acknowledgements
This research was partially supported by the JAIST Research Grant for Fundamental Research, by JSPS KAKENHI Grant Numbers JP23K28060, JP23K19959, JP24K20757, and JP24KK0185.
