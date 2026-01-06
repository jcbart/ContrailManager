# Contrail Manager

The **Contrail Manager** is a library for simulating contrails with online feedback to a NWP model using the [ESMF](https://earthsystemmodeling.org/) library.

## Dependencies

- CMake
- [ESMF](https://earthsystemmodeling.org/)
- [yaml-cpp](https://github.com/jbeder/yaml-cpp)

CMake and ESMF must be installed on your system.

yaml-cpp is a submodule so will compile within the **Contrail Manager**.

## Compilation

If compiling for the first time, clone the Git repo and run
```bash
git submodule update --init --recursive
```
to add the submodules.

Like any ESMF project, the **Contrail Manager** requires that the environment variable `ESMFMKFILE` points to the `esmf.mk` file created during the installation of ESMF. CMake will search for this file and extract the relevant variables.

Run the following commands to compile in the `build` directory:
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

Successful compilation will produce the static library `build/libcontrailmanager.a`.

## Use

The **Contrail Manager** is designed to be incorporated as a component in an ESMF coupled model. For a coupling with WRF, see [WRF-Contrail-Coupler](https://github.com/jcbart/WRF-Contrail-Coupler).

Coupling the **Contrail Manager** with a model requires calling the functions in `src/extern.cpp`. For fields that are updated independently by both the **Contrail Manager** and a NWP, the **Contrail Manager** writes changes to a delta field (e.g. `deltaQV`) so at the end of the coupling interval, the NWP's internal field can be updated with e.g.
```
QV = QV + deltaQV
```
which allows the models to run simultaneously.

Contrail ice mass in segments which are still alive (i.e. have not reached an age or dissipation threshold) is written to `QIcontrail` at the end of each coupling interval.

For examples of Fortran interfaces for the **Contrail Manager** functions, see [WRF-Contrail-Coupler](https://github.com/jcbart/WRF-Contrail-Coupler).

Runtime configuration options are set in `CM-config.yaml`.