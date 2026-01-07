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
    CMptr->domain.proj.init(proj_code, lat1, lon1, knowni, knownj, dx, stdlon, truelat1, truelat2);
}

// Variable initialisation
extern "C" void init_vars_extern(ContrailManager* CMptr, int ids, int ide, int jds, int jde, int kds, int kde) {
    CMptr->domain.init_vars(ids, ide, jds, jde, kds, kde);
}


// Variable getters

// Returns a pointer to the XLAT data
extern "C" float* get_XLAT_extern(ContrailManager* CMptr) {
    return CMptr->domain.XLAT.data;
}

// Returns a pointer to the XLONG data
extern "C" float* get_XLONG_extern(ContrailManager* CMptr) {
    return CMptr->domain.XLONG.data;
}

// Returns a pointer to the Z data
extern "C" float* get_Z_extern(ContrailManager* CMptr) {
    return CMptr->domain.Z.data;
}

// Returns a pointer to the Z_AT_W data
extern "C" float* get_Z_AT_W_extern(ContrailManager* CMptr) {
    return CMptr->domain.Z_AT_W.data;
}

// Returns a pointer to the DRYMASS data
extern "C" float* get_DRYMASS_extern(ContrailManager* CMptr) {
    return CMptr->domain.DRYMASS.data;
}

// Returns a pointer to the T_POT data
extern "C" float* get_T_POT_extern(ContrailManager* CMptr) {
    return CMptr->domain.T_POT.data;
}

// Returns a pointer to the P data
extern "C" float* get_P_extern(ContrailManager* CMptr) {
    return CMptr->domain.P.data;
}

// Returns a pointer to the U data
extern "C" float* get_U_extern(ContrailManager* CMptr) {
    return CMptr->domain.U.data;
}

// Returns a pointer to the V data
extern "C" float* get_V_extern(ContrailManager* CMptr) {
    return CMptr->domain.V.data;
}

// Returns a pointer to the W data
extern "C" float* get_W_extern(ContrailManager* CMptr) {
    return CMptr->domain.W.data;
}

// Returns a pointer to the QV data
extern "C" float* get_QV_extern(ContrailManager* CMptr) {
    return CMptr->domain.QV.data;
}

// Returns a pointer to the deltaQV data
extern "C" float* get_deltaQV_extern(ContrailManager* CMptr) {
    return CMptr->domain.deltaQV.data;
}

// Returns a pointer to the deltaQI data
extern "C" float* get_deltaQI_extern(ContrailManager* CMptr) {
    return CMptr->domain.deltaQI.data;
}

// Returns a pointer to the deltaNI data
extern "C" float* get_deltaNI_extern(ContrailManager* CMptr) {
    return CMptr->domain.deltaNI.data;
}

// Returns a pointer to the QIcontrail data
extern "C" float* get_QIcontrail_extern(ContrailManager* CMptr) {
    return CMptr->domain.QIcontrail.data;
}