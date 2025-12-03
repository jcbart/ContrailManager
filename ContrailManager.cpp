#include <ESMC.h>
#include <iostream>
#include <string>
#include <nanoflann.hpp>
#include "ContrailManager.h"
#include "timekeeping.h"
#include "variables.h"
#include "segment.h"
#include "flight.h"
#include "mapUtils.h"

void ContrailManager::init() {
    int rc;
    std::string msg;
    rc = ESMC_LogWrite("Initialising Contrail Manager:", ESMC_LOGMSG_INFO);
    int timeStep_s = 10; // Read from CM config
    timeStep.set(0, 0, 0, 0, 0, timeStep_s);
    msg = "Contrail Manager internal time step set to " + timeStep.asString();
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
    // Read max_initial_seg_length from CM config
    // Read flight data etc
    rc = ESMC_LogWrite("Contrail Manager initialised", ESMC_LOGMSG_INFO);

    std::cerr << "timeStep in init: " << timeStep.asString()  << std::endl;
}

void ContrailManager::init_vars(int ids, int ide, int jds, int jde, int kds, int kde) {
    XLAT.init("XLAT", ids, ide, jds, jde);
    XLONG.init("XLONG", ids, ide, jds, jde);
    Z.init("Z", ids, ide, jds, jde, kds, kde);
    U.init("U", ids, ide, jds, jde, kds, kde);
    V.init("V", ids, ide, jds, jde, kds, kde);
    W.init("W", ids, ide, jds, jde, kds, kde);
    QV.init("QV", ids, ide, jds, jde, kds, kde);
}

// Integrate between times
void ContrailManager::run(CMTime& startTime, CMTime& stopTime) {
    int rc;
    std::string msg;

    std::cerr << "timeStep in run: " << timeStep.asString()  << std::endl;
    std::cerr << "firstRunCall = " << firstRunCall << std::endl;

    if (firstRunCall) {
        currTime = startTime;
        msg = "Contrail Manager current time set to " + currTime.asString();
        rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);

        // Take sizes from Z and hope they are all consistent
        ids = Z.get_ids();
        ide = Z.get_ide();
        jds = Z.get_jds();
        jde = Z.get_jde();
        kds = Z.get_kds();
        kde = Z.get_kde();
        latSize = Z.get_i_size();
        lonSize = Z.get_j_size();
        altSize = Z.get_k_size();

        // Setup k-d tree
        setup_kdtree();
        msg = "k-d tree set up";
        rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
    }
    firstRunCall = false;
    
    // Check startTime matches expected time
    if (currTime != startTime) {
        std::cerr << "Error: currTime (" << currTime.asString() << ") does not match "
                << "integration startTime (" << startTime.asString() << ")" << std::endl;
        exit(EXIT_FAILURE);
    }

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

    msg = "Z(100,200,10) = " + std::to_string(*Z.get(100, 200, 10));
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);

    msg = "U(100,200,10) = " + std::to_string(*U.get(100, 200, 10));
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);

    msg = "V(100,200,10) = " + std::to_string(*V.get(100, 200, 10));
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);

    msg = "W(100,200,10) = " + std::to_string(*W.get(100, 200, 10));
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);

    /*
    Geo2D my_point = {50.7, 1.25};
    std::vector<uint32_t> out_indices(1);
    std::vector<float>    out_dist_sqr(1);
    geoIndexer.search(my_point, 1, out_indices, out_dist_sqr);
    Cart3D nn = geoIndexer.cloud.points[out_indices[0]];
    Geo2D nnGeo = Cart3D_to_Geo2D(nn);
    msg = "Nearest neighbour is lat = " + std::to_string(nnGeo.lat) + ", lon = " + std::to_string(nnGeo.lon);
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
    IDX2 closest_ij = PointCloud_idx_to_ij(out_indices[0]);
    msg = "Closest point to (50.7, 1.25) is i = " + std::to_string(closest_ij.i) + ", j = " + std::to_string(closest_ij.j);
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
    msg = "which is XLAT(i,j) = " + std::to_string(*XLAT.get(closest_ij.i, closest_ij.j)) + ", XLONG(i,j) = " + std::to_string(*XLONG.get(closest_ij.i, closest_ij.j));
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
    */
    
    currTime = startTime;
    while (currTime+timeStep <= stopTime) {
        currTime += timeStep;
        msg = "Current time: " + currTime.asString();
        rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
        // Do stuff

        /*
        1. Create new segments
        2. Integrate all segments (aggregate vapour delta); mark dead segments
        3. Advect all segments
        4. Dump dead segments (aggregate leftover crystals)
        */
    }
}

void ContrailManager::setup_kdtree() {
    geoIndexer.cloud.points.resize(latSize*lonSize);
    // Add all points to geoIndexer's PointCloud
    uint32_t idx = 0;
    for (int i = ids; i <= ide; i++) {
        for (int j = jds; j <= jde; j++) {
            Geo2D loc;
            loc.lat = *XLAT.get(i, j);
            loc.lon = *XLONG.get(i, j);
            geoIndexer.cloud.points[idx++] = Geo2D_to_Cart3D(loc);
        }
    }

    // Build the k-d tree
    geoIndexer.buildIndex();
}

IDX2 ContrailManager::PointCloud_idx_to_ij(uint32_t idx) {
    IDX2 ij;
    ij.i = ids + idx / lonSize;
    ij.j = jds + idx % lonSize;
    return ij;
}

int ContrailManager::find_corner_k(Geo3D& loc, IDX2& ij) {
    for (int k = 0; k < altSize; k++) {
        if (loc.alt >= *Z.get(ij.i, ij.j, k) && loc.alt < *Z.get(ij.i, ij.j, k+1)) {
            return k;
        }
    }
    return -1;
}