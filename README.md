# FUnTiDES: Fast Unstructured Time Dynamic Equation Solver


## Quick Start: Build and Run

### Step 1: Compile and Install

Pour clone kokkos dans notre projet
```
git submodule update --init
```

```sh
mkdir build
cd build
cmake -DENABLE_CUDA=ON -DUSE_KOKKOS=ON -DUSE_VECTOR=OFF ..
make 
```

By default, this builds the applications in sequential mode using `std::vector`.
Both SEM and FD applications are compiled.

### Step 2: Run Examples

```sh
# Run SEM simulation with 100 x 100 x 100 elements
./src/main/semproxy -ex 100

# Run FD simulation
./src/main/fdproxy
```

---

## CMake Options

The following options can be used to configure your build:

| Option                 | Description                                                                 |
|------------------------|-----------------------------------------------------------------------------|
| `COMPILE_FD`           | Enable compilation of the FD proxy (default: ON)                            |
| `COMPILE_SEM`          | Enable compilation of the SEM proxy (default: ON)                           |
| `ENABLE_CUDA`          | Enable CUDA backend (used by Kokkos)                                        |
| `ENABLE_PYWRAP`        | Enable Python bindings via pybind11 (experimental)                          |
| `USE_KOKKOS`           | Enable Kokkos support (serial by default, CUDA/OpenMP with flags)           |
| `USE_VECTOR`           | Use `std::vector` for data arrays (enabled by default unless Kokkos is used)|

---

## Parameters
- --snapshot : Enable or disable saving snapshots
- --sismo-points PATH : Path to sismo receptors points to save
- --sismo-fourier PATH: Path to sismo receptors points to do fourier on
- --stats-analysis : Enable stats analysis in-situ
- --compute-histogram : Enable or disable computing histogram for pressure value distribution
- --compute-histogram-delay DELAY : Delay between each histogram computation (step (ms) )
- --sd DELAY : Delay between each snapshot (step (ms) )
- --slice-snapshot COORD : Enable snapshots at given coordinates. Will use the --snapshot-delay parameter for delay
- --slice-ppm : Save slice snapshots as PPM format. Saving slice snapshots without this option result in regular save (ASCII format)


## Run benchmarks

Commands to run the benchmarks are specified in the PDF report.