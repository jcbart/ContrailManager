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

extern "C" void ContrailManager_run_extern(ContrailManager* CMptr, CMTime_F startTime_F, CMTime_F stopTime_F) {
    CMTime startTime, stopTime;
    startTime.set(startTime_F);
    stopTime.set(stopTime_F);
    CMptr->run(startTime, stopTime);
}

// Projection setup

extern "C" void init_projection_extern(ContrailManager* CMptr, int proj_code, float lat1, float lon1, float knowni, float knownj, float dx, float stdlon, float truelat1, float truelat2) {
    CMptr->proj.init(proj_code, lat1, lon1, knowni, knownj, dx, stdlon, truelat1, truelat2);
}

// Variable initialisation
extern "C" void init_vars_extern(ContrailManager* CMptr, int ids, int ide, int jds, int jde, int kds, int kde) {
    CMptr->init_vars(ids, ide, jds, jde, kds, kde);
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

// Returns a reference to the Z_AT_W value, so can be used to set and get
extern "C" float* get_Z_AT_W_element_extern(ContrailManager* CMptr, int i, int j, int k) {
    return CMptr->Z_AT_W.get(i, j, k);
}

// Returns a reference to the U value, so can be used to set and get
extern "C" float* get_U_element_extern(ContrailManager* CMptr, int i, int j, int k) {
    return CMptr->U.get(i, j, k);
}

// Returns a reference to the V value, so can be used to set and get
extern "C" float* get_V_element_extern(ContrailManager* CMptr, int i, int j, int k) {
    return CMptr->V.get(i, j, k);
}

// Returns a reference to the W value, so can be used to set and get
extern "C" float* get_W_element_extern(ContrailManager* CMptr, int i, int j, int k) {
    return CMptr->W.get(i, j, k);
}

// Returns a reference to the QV value, so can be used to set and get
extern "C" float* get_QV_element_extern(ContrailManager* CMptr, int i, int j, int k) {
    return CMptr->QV.get(i, j, k);
}