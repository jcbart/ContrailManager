#include <ESMC.h>
#include <iostream>
#include <string>
#include "ContrailManager.h"
#include "timekeeping.h"
#include "variables.h"
#include "segment.h"

void ContrailManager::init() {
    int rc;
    std::string msg;
    rc = ESMC_LogWrite("Initialising Contrail Manager:", ESMC_LOGMSG_INFO);
    int timeStep_s = 10; // Read from CM config
    timeStep.set(0, 0, 0, 0, 0, timeStep_s);
    msg = "Contrail Manager internal time step set to " + timeStep.asString();
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
    // Read flight data etc
    rc = ESMC_LogWrite("Contrail Manager initialised", ESMC_LOGMSG_INFO);

    std::cerr << "timeStep in init: " << timeStep.asString()  << std::endl;
}

// Should be called before run() to set the start time
// If not, checking startTime against currTime in run() will fail
void ContrailManager::setStartTime(CMTime& startTime) {
    int rc;
    std::string msg;
    currTime = startTime;
    msg = "Contrail Manager internal time set to " + currTime.asString();
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);

    std::cerr << "timeStep in setStartTime: " << timeStep.asString()  << std::endl;
}

// Integrate between times
void ContrailManager::run(CMTime& startTime, CMTime& stopTime) {
    int rc;
    std::string msg;
    bool anyDummies = false;

    if (dummy1 != 0) {std::cerr << "dummy1 = " << dummy1 << std::endl; anyDummies=true;}
    if (dummy2 != 0) {std::cerr << "dummy2 = " << dummy2 << std::endl; anyDummies=true;}
    if (dummy3 != 0) {std::cerr << "dummy3 = " << dummy3 << std::endl; anyDummies=true;}
    if (dummy4 != 0) {std::cerr << "dummy4 = " << dummy4 << std::endl; anyDummies=true;}
    if (dummy5 != 0) {std::cerr << "dummy5 = " << dummy5 << std::endl; anyDummies=true;}
    if (dummy6 != 0) {std::cerr << "dummy6 = " << dummy6 << std::endl; anyDummies=true;}
    if (dummy7 != 0) {std::cerr << "dummy7 = " << dummy7 << std::endl; anyDummies=true;}
    if (dummy8 != 0) {std::cerr << "dummy8 = " << dummy8 << std::endl; anyDummies=true;}
    if (anyDummies) {exit(EXIT_FAILURE);}

    std::cerr << "timeStep in run: " << timeStep.asString()  << std::endl;
    std::cerr << "firstRunCall = " << firstRunCall << std::endl;

    if (firstRunCall) {
        currTime = startTime;
        msg = "Contrail Manager current time set to " + currTime.asString();
        rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);

        // Take sizes from Z and hope they are all consistent
        latSize = Z.get_i_size();
        lonSize = Z.get_j_size();
        altSize = Z.get_k_size();
    }
    firstRunCall = false;
    
    // Check startTime matches expected time
    if (currTime != startTime) {
        std::cerr << "Error: currTime (" << currTime.asString() << ") does not match "
                << "integration startTime (" << startTime.asString() << ")" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Temp
    timeStep.set(0, 0, 0, 0, 0, 10);

    // Check there are a whole number of time steps between startTime and stopTime
    CMTimeInterval timeInterval = stopTime-startTime;
    if (timeInterval.dhms_to_s() % timeStep.dhms_to_s() != 0) {
        std::cerr << "Error: Integration time interval (" << timeInterval.asString()
                  << ") is not an integer multiple of time step ("
                  << timeStep.asString() << ")" << std::endl;
        exit(EXIT_FAILURE);
    }

    msg = "Integrating between " + startTime.asString() + " and "
          + stopTime.asString();
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);

    msg = "XLAT(1,1) = " + std::to_string(*XLAT.get(1, 1));
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);

    msg = "Z(100,200,10) = " + std::to_string(*Z.get(100, 200, 10));
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);

    currTime = startTime;
    while (currTime+timeStep <= stopTime) {
        currTime += timeStep;
        msg = "Current time: " + currTime.asString();
        rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
        // Do stuff

        /*
        1. Create new segments
        2. Integrate all segments (aggregate vapour delta)
        3. Dump old segments (aggregate leftover crystals)
        4. Advect remaining segments
        */
    }
}

void ContrailManager::init_XLAT(int ids, int ide, int jds, int jde) {
    XLAT.init(ids, ide, jds, jde);
}

void ContrailManager::init_XLONG(int ids, int ide, int jds, int jde) {
    XLONG.init(ids, ide, jds, jde);
}

void ContrailManager::init_Z(int ids, int ide, int jds, int jde, int kds, int kde) {
    Z.init(ids, ide, jds, jde, kds, kde);
}

IDX3 ContrailManager::find_corner_idxs(Location& loc) {
    IDX2 ij;
    IDX3 idxs;
    idxs.i = -1;
    idxs.j = -1;
    idxs.k = -1;
    ij = find_ij(loc);
    idxs.i = ij.i;
    idxs.j = ij.j;
    // Search column for k - should really check all 4 corners?
    idxs.k = find_k(loc, ij);
    return idxs;
}

IDX2 ContrailManager::find_ij(Location& loc) {
    IDX2 ij;
    ij.i = -1;
    ij.j = -1;
    for (int i = 0; i < latSize; i++) {
        for (int j = 0; j < lonSize; j++) {
            if (loc.lat >= *XLAT.get(i, j) &&
                loc.lon >= *XLONG.get(i, j) &&
                loc.lat < *XLAT.get(i+1, j+1) &&
                loc.lon < *XLONG.get(i+1, j+1)) {
                    ij.i = i;
                    ij.j = j;
                    return ij;
            }
        }
    }
    return ij;
}

int ContrailManager::find_k(Location& loc, IDX2& ij) {
    for (int k = 0; k < altSize; k++) {
        if (loc.alt >= *Z.get(ij.i, ij.j, k) && loc.alt < *Z.get(ij.i, ij.j, k+1)) {
            return k;
        }
    }
    return -1;
}