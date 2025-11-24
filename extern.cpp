// External interfaces

#include "ContrailManager.h"

// Create and return reference to the contrail manager
extern "C" ContrailManager* create_ContrailManager() {
    return new ContrailManager;
}

extern "C" void ContrailManager_init_extern(ContrailManager* CMptr) {
    CMptr->init();
}

extern "C" void ContrailManager_setStartTime_extern(ContrailManager* CMptr, CMTime& startTime) {
    CMptr->setStartTime(startTime);
}

extern "C" void ContrailManager_run_extern(ContrailManager* CMptr, CMTime_F startTime_F, CMTime_F stopTime_F) {
    CMTime startTime, stopTime;
    startTime.set(startTime_F);
    stopTime.set(stopTime_F);
    CMptr->run(startTime, stopTime);
}


// Variables inits

extern "C" void init_XLAT_extern(ContrailManager* CMptr, int ids, int ide, int jds, int jde) {
    CMptr->init_XLAT(ids, ide, jds, jde);
}

extern "C" void init_XLONG_extern(ContrailManager* CMptr, int ids, int ide, int jds, int jde) {
    CMptr->init_XLONG(ids, ide, jds, jde);
}

extern "C" void init_Z_extern(ContrailManager* CMptr, int ids, int ide, int jds, int jde, int kds, int kde) {
    CMptr->init_Z(ids, ide, jds, jde, kds, kde);
}


// Variable getters

// Returns a reference to the XLAT value, so can be used to set and get
extern "C" float* get_XLAT_element_extern(ContrailManager* CMptr, int i, int j) {
    return CMptr->XLAT.get(i, j);
}

// Returns a reference to the XLONG value, so can be used to set and get
extern "C" float* get_XLONG_element_extern(ContrailManager* CMptr, int i, int j) {
    return CMptr->XLONG.get(i, j);
}

// Returns a reference to the Z value, so can be used to set and get
extern "C" float* get_Z_element_extern(ContrailManager* CMptr, int i, int j, int k) {
    return CMptr->Z.get(i, j, k);
}