// External interfaces

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

// Returns a reference to the DRYMASS value, so can be used to set and get
extern "C" float* get_DRYMASS_element_extern(ContrailManager* CMptr, int i, int j, int k) {
    return CMptr->DRYMASS.get(i, j, k);
}

// Returns a reference to the T_POT value, so can be used to set and get
extern "C" float* get_T_POT_element_extern(ContrailManager* CMptr, int i, int j, int k) {
    return CMptr->T_POT.get(i, j, k);
}

// Returns a reference to the P value, so can be used to set and get
extern "C" float* get_P_element_extern(ContrailManager* CMptr, int i, int j, int k) {
    return CMptr->P.get(i, j, k);
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

// Returns a reference to the deltaQV value, so can be used to set and get
extern "C" float* get_deltaQV_element_extern(ContrailManager* CMptr, int i, int j, int k) {
    return CMptr->deltaQV.get(i, j, k);
}

// Returns a reference to the QI value, so can be used to set and get
//extern "C" float* get_QI_element_extern(ContrailManager* CMptr, int i, int j, int k) {
//    return CMptr->QI.get(i, j, k);
//}

// Returns a reference to the deltaQI value, so can be used to set and get
extern "C" float* get_deltaQI_element_extern(ContrailManager* CMptr, int i, int j, int k) {
    return CMptr->deltaQI.get(i, j, k);
}

// Returns a reference to the NI value, so can be used to set and get
//extern "C" float* get_NI_element_extern(ContrailManager* CMptr, int i, int j, int k) {
//    return CMptr->NI.get(i, j, k);
//}

// Returns a reference to the deltaNI value, so can be used to set and get
extern "C" float* get_deltaNI_element_extern(ContrailManager* CMptr, int i, int j, int k) {
    return CMptr->deltaNI.get(i, j, k);
}

// Returns a reference to the QIcon value, so can be used to set and get
extern "C" float* get_QIcon_element_extern(ContrailManager* CMptr, int i, int j, int k) {
    return CMptr->QIcon.get(i, j, k);
}

// Returns a reference to the NIcon value, so can be used to set and get
extern "C" float* get_NIcon_element_extern(ContrailManager* CMptr, int i, int j, int k) {
    return CMptr->NIcon.get(i, j, k);
}