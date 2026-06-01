# Contrail Manager

The **Contrail Manager** is a library for simulating contrails with online one-way or two-way feedback to a NWP model using the [ESMF](https://earthsystemmodeling.org/) library.

The contrail plume models currently integrated with the **Contrail Manager** are:
- **Contrails as Cloud Enhancement (CaCE):** calculates contrail formation and the wake vortex similar to CoCiP, then immediately releases the contrail to the NWP; has no effect if coupling is one-way.
- **Contrail Cirrus Prediction Tool (CoCiP):** as first described in [Schumann (2012)](https://doi.org/10.5194/gmd-5-543-2012); see [CoCiP++](https://github.com/jcbart/CoCiPPlusPlus).

## Requirements

System requirements:
- C++ compiler supporting C++20
- OpenMP (likely included with compiler)
- CMake
- Git
- [ESMF](https://earthsystemmodeling.org/)

Git submodules:
- [CoCiP++](https://github.com/jcbart/CoCiPPlusPlus) (optional)
- [vcpkg](https://github.com/microsoft/vcpkg)

vcpkg packages:
- Arrow and its many dependencies
- Cereal
- yaml-cpp

Only the system requirements must be installed beforehand. Everything else will be built with the **Contrail Manager**.

## Installation

If installing for the first time, clone the Git repo and run
```bash
git submodule update --init --recursive
```
to add the submodules. It is not required to add submodules for optional plume models; see below.

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
- `-DBUILD_TOOLS=OFF`: Do not build tools (see [Tools](#tools); default is `-DBUILD_TOOLS=ON`).

CMake will trigger vcpkg to find the required libraries. If vcpkg has not already installed the libraries on your system, it will build and then cache them.

> [!NOTE]
> The Arrow library may take a long time to build since it has many dependencies even when installed with only basic features. If desired, use multiple threads with `export VCPKG_MAX_CONCURRENCY=[N]` or equivalent in your environment to expedite the process.

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

### Integrating with a NWP

The **Contrail Manager** is designed to be incorporated as a component in an ESMF coupled model. For a coupling with WRF, see [WRFContrail](https://github.com/jcbart/WRFContrail).

Coupling the **Contrail Manager** with a model requires calling the functions in `src/extern.cpp`. For fields that are updated independently by both the **Contrail Manager** and a NWP, the **Contrail Manager** writes changes to a delta field (e.g. `delta_QV`) so at the end of the coupling interval, the NWP's internal field can be updated with e.g.
```
QV = QV + delta_QV
```
which allows the models to run simultaneously.

The **Contrail Manager** also provides tendency equivalents for delta fields which are the deltas divided by the coupling interval duration.

Contrail ice mass in segments which are still alive (i.e. have not reached an age, size, or dissipation threshold) is written to `QIcontrail` at the end of each coupling interval. The combined ice crystal effective radius is written to `REIcontrail`.

For examples of Fortran interfaces for the **Contrail Manager** functions, see [WRFContrail](https://github.com/jcbart/WRFContrail).

### Running

Runtime configuration options are set in `CM-config.yaml` which must be located in the same directory as the final executable along with any input files required by the contrail plume models.

The **Contrail Manager** will produce binary files of serialized segments which can be post-processed using the [tools](#tools) or read back into the program if restarting. Binary files will not be produced for the CaCE plume model since no segments can exist at the end of a coupling interval.

Flight data must be provided in separate files of waypoint and aircraft data.

Waypoint data must include:
- Flight ID
- Time
- Longitude (degrees), latitude (degrees), and altitude (m)
- Aircraft mass (kg)
- Fuel flow (kg s-1)
- Engine efficiency ()
- Number emission index (EI) of non-volatile particulate matter (nvPM) (# kg-1)

Aircraft data must include:
- Flight ID
- Wingspan (m)

Currently, the emission index of water vapour and the fuel specific combustion heat are fixed at 1.25 kg kg-1 and 43.15 MJ kg-1, respectively.

Only Parquet file reading has been implemented so far.

## Tools

Unless configured with `-DBUILD_TOOLS=OFF`, CMake will compile and install additional tools for the **Contrail Manager**.

| Name       | Description                                        | Usage                                              |
| ---------- | -------------------------------------------------- | -------------------------------------------------- |
| `bintocsv` | Converts binary output files to CSV summaries.     | `./bintocsv output.bin` will produce `output.csv`. |
| `bintopq`  | Converts binary output files to Parquet summaries. | `./bintopq output.bin` will produce `output.pq`.   |