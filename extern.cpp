// External interfaces

#include <iostream>
#include "ContrailManager.h"

// Create and return reference to the contrail manager
extern "C" ContrailManager* create_ContrailManager() {
    return new ContrailManager;
}

extern "C" void ContrailManager_init_extern(ContrailManager* CMptr) {
    CMptr->init();
}

// The times are references which Fortran can handle because it passes derived types as references by default
// However, rc is an integer (an intrinsic type), so must get Fortran to pass a pointer with c_loc(rc)
extern "C" void ContrailManager_run_extern(ContrailManager* CMptr, CMTime& startTime, CMTime& stopTime) {
    CMptr->run(startTime, stopTime);
}

extern "C" void init_XLAT_extern(ContrailManager* CMptr, int ids, int ide, int jds, int jde) {
    CMptr->init_XLAT(ids, ide, jds, jde);
}

extern "C" void init_XLONG_extern(ContrailManager* CMptr, int ids, int ide, int jds, int jde) {
    CMptr->init_XLONG(ids, ide, jds, jde);
}

// Returns a reference to the XLAT value, so can be used to set and get
extern "C" float* get_XLAT_element_extern(ContrailManager* CMptr, int i, int j) {
    return CMptr->XLAT.get(i, j);
}

// Returns a reference to the XLONG value, so can be used to set and get
extern "C" float* get_XLONG_element_extern(ContrailManager* CMptr, int i, int j) {
    return CMptr->XLONG.get(i, j);
}