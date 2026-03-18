# Contrail Manager

The **Contrail Manager** is a library for simulating contrails with online feedback to a NWP model using the [ESMF](https://earthsystemmodeling.org/) library.

## Requirements

System requirements:
- C++ compiler supporting C++20
- CMake
- Git
- [ESMF](https://earthsystemmodeling.org/)

Git submodules:
- [CoCiP++](https://github.com/jcbart/CoCiPPlusPlus) (optional)
- [vcpkg](https://github.com/microsoft/vcpkg)

vcpkg packages:
- Arrow and its many dependencies
- yaml-cpp

Only the system requirements must be installed beforehand. Everything else will be built with the **Contrail Manager**.

## Installation

If installing for the first time, clone the Git repo and run
```bash
git submodule update --init --recursive
```
to add the submodules. The contrail plume models are optional; see below.

From the top directory, set up vcpkg with
```bash
./submodules/vcpkg/bootstrap-vcpkg.sh -disableMetrics
```
or the equivalent for non-Linux systems.

Like any ESMF project, the **Contrail Manager** requires that the environment variable `ESMFMKFILE` points to the `esmf.mk` file created during the installation of ESMF. CMake will search for this file and extract the relevant variables.

From the top directory, run the following commands to configure in a directory named `build`:
```bash
mkdir build
cd build
cmake ..
```

Configuration options that can be used in the `cmake` command are listed below:
- `-DCMAKE_BUILD_TYPE=Debug`: Compile with debug flags and no optimisation (default is `-DCMAKE_BUILD_TYPE=Release`).
- `-DWITH_COCIP=OFF`: Do not build with CoCiP (default is `-DWITH_COCIP=ON`).

CMake will trigger vcpkg to find the required libraries. If vcpkg has not already installed the libraries on your system, it will build them and then cache them.

> [!NOTE]
> The Arrow library may take a long time to build since it has many dependencies even when installed with only basic features. If desired, use multiple threads with `export VCPKG_MAX_CONCURRENCY=...` or equivalent in your environment to expedite the process.

Build the **Contrail Manager** with
```bash
cmake --build .
```
then install with
```bash
cmake --install . --prefix /my/install/prefix
```

If `--prefix` is not passed, the **Contrail Manager** will be installed in the build directory.

## Use

The **Contrail Manager** is designed to be incorporated as a component in an ESMF coupled model. For a coupling with WRF, see [WRFContrail](https://github.com/jcbart/WRFContrail).

Coupling the **Contrail Manager** with a model requires calling the functions in `src/extern.cpp`. For fields that are updated independently by both the **Contrail Manager** and a NWP, the **Contrail Manager** writes changes to a delta field (e.g. `deltaQV`) so at the end of the coupling interval, the NWP's internal field can be updated with e.g.
```
QV = QV + deltaQV
```
which allows the models to run simultaneously.

Contrail ice mass in segments which are still alive (i.e. have not reached an age or dissipation threshold) is written to `QIcontrail` at the end of each coupling interval.

For examples of Fortran interfaces for the **Contrail Manager** functions, see [WRF-Contrail-Coupler](https://github.com/jcbart/WRFContrail).

Runtime configuration options are set in `CM-config.yaml` which must be located in the same directory as the final executable along with any input files required by the contrail plume models.