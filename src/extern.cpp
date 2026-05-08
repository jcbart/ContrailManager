// External interfaces

#include <memory>
#include "ContrailManager.h"

// Create and return reference to the Contrail Manager
extern "C" ContrailManager* create_ContrailManager() {
    return new ContrailManager;
}

// Initialise Contrail Manager
extern "C" void ContrailManager_init_extern(ContrailManager* CMptr) {
    CMptr->init();
}

// Run Contrail Manager between startTime and stopTime
extern "C" void ContrailManager_run_extern(ContrailManager* CMptr, CMTime_F startTime_F,
    CMTime_F stopTime_F) {
    
    CMTime startTime, stopTime;
    startTime.set(startTime_F);
    stopTime.set(stopTime_F);
    CMptr->run(startTime, stopTime);
}

// Domain with Lambert-Conformal projection initialisation
extern "C" void init_domainlc_extern(ContrailManager* CMptr, int ids, int ide, int jds,
    int jde, int kds, int kde, float lat1, float lon1, float knowni, float knownj, float dx,
    float stdlon, float truelat1, float truelat2) {

    ProjectionLC proj(lat1, lon1, knowni, knownj, dx, stdlon, truelat1, truelat2);

    CMptr->domain = std::make_unique<Domain>(ids, ide, jds, jde, kds, kde, proj);
}


// Variable getters

// Returns a pointer to the XLAT data
extern "C" float* get_XLAT_extern(ContrailManager* CMptr) {
    return CMptr->domain->XLAT.get_data();
}

// Returns a pointer to the XLONG data
extern "C" float* get_XLONG_extern(ContrailManager* CMptr) {
    return CMptr->domain->XLONG.get_data();
}

// Returns a pointer to the Z data
extern "C" float* get_Z_extern(ContrailManager* CMptr) {
    return CMptr->domain->Z.get_data();
}

// Returns a pointer to the Z_AT_W data
extern "C" float* get_Z_AT_W_extern(ContrailManager* CMptr) {
    return CMptr->domain->Z_AT_W.get_data();
}

// Returns a pointer to the DRYMASS data
extern "C" float* get_DRYMASS_extern(ContrailManager* CMptr) {
    return CMptr->domain->DRYMASS.get_data();
}

// Returns a pointer to the T_POT data
extern "C" float* get_T_POT_extern(ContrailManager* CMptr) {
    return CMptr->domain->T_POT.get_data();
}

// Returns a pointer to the P data
extern "C" float* get_P_extern(ContrailManager* CMptr) {
    return CMptr->domain->P.get_data();
}

// Returns a pointer to the U data
extern "C" float* get_U_extern(ContrailManager* CMptr) {
    return CMptr->domain->U.get_data();
}

// Returns a pointer to the V data
extern "C" float* get_V_extern(ContrailManager* CMptr) {
    return CMptr->domain->V.get_data();
}

// Returns a pointer to the W data
extern "C" float* get_W_extern(ContrailManager* CMptr) {
    return CMptr->domain->W.get_data();
}

// Returns a pointer to the TNSR data
extern "C" float* get_TNSR_extern(ContrailManager* CMptr) {
    return CMptr->domain->TNSR.get_data();
}

// Returns a pointer to the OLR data
extern "C" float* get_OLR_extern(ContrailManager* CMptr) {
    return CMptr->domain->OLR.get_data();
}

// Returns a pointer to the QV data
extern "C" float* get_QV_extern(ContrailManager* CMptr) {
    return CMptr->domain->QV.get_data();
}

// Returns a pointer to the deltaQV data
extern "C" float* get_deltaQV_extern(ContrailManager* CMptr) {
    return CMptr->domain->deltaQV.get_data();
}

// Returns a pointer to the QI data
extern "C" float* get_QI_extern(ContrailManager* CMptr) {
    return CMptr->domain->QI.get_data();
}

// Returns a pointer to the deltaQI data
extern "C" float* get_deltaQI_extern(ContrailManager* CMptr) {
    return CMptr->domain->deltaQI.get_data();
}

// Returns a pointer to the deltaNI data
extern "C" float* get_deltaNI_extern(ContrailManager* CMptr) {
    return CMptr->domain->deltaNI.get_data();
}

// Returns a pointer to the QIcontrail data
extern "C" float* get_QIcontrail_extern(ContrailManager* CMptr) {
    return CMptr->domain->QIcontrail.get_data();
}

// Returns a pointer to the REIcontrail data
extern "C" float* get_REIcontrail_extern(ContrailManager* CMptr) {
    return CMptr->domain->REIcontrail.get_data();
}