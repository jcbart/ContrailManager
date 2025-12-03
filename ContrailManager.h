#ifndef CONTRAILMANAGER
#define CONTRAILMANAGER

#include <vector>
#include "timekeeping.h"
#include "variables.h"
#include "segment.h"
#include "flight.h"
#include "kdtree.h"
#include "mapUtils.h"

class ContrailManager {
private:
    CMTimeInterval timeStep;
    CMTime currTime;
    bool firstRunCall = true;
    float max_initial_seg_len = 2500; // Maximum length of a new segment (m)
    // End indices are one smaller than in WRF because CM does not use a staggered grid
    int ids, ide, jds, jde, kds, kde = 0;
    int latSize, lonSize, altSize = 0;

public:
    // Meteorological variables
    Variable2D XLAT; // Latitude (degrees, South is negative)
    Variable2D XLONG; // Longitude (degrees, West is negative)
    Variable3D Z; // Height above sea level at cell centre (m)
    Variable3D T; // Temperature (K)
    Variable3D P; // Pressure (Pa)
    Variable3D U; // Wind speed in lon direction (m s-1)
    Variable3D V; // Wind speed in lat direction (m s-1)
    Variable3D W; // Wind speed in vertical direction (m s-1)
    Variable3D QV; // Water vapour mass mixing ratio (kg kg-1)
    Variable3D deltaQV; // Change in water vapour mass mixing ratio (kg kg-1)
    Variable3D QI; // Ice mass mixing ratio excl. live contrails (kg kg-1)
    Variable3D deltaQI; // Change in ice mass mixing ratio excl. live contrails (kg kg-1)
    Variable3D NI; // Ice number mixing ratio excl. live contrails (kg kg-1)
    Variable3D deltaNI; // Change in ice number mixing ratio excl. live contrails (kg kg-1)
    Variable3D QIcon; // Contrail ice mass mixing ratio (kg kg-1)
    Variable3D NIcon; // Contrail ice number mixing ratio (kg kg-1)

    // Flight vector
    std::vector<Flight> flights;

    // Contrail segment vector
    std::vector<Segment> segments;

    // k-d tree for finding nearest neighbours in lat/lon grid
    KDTreeIndexer geoIndexer;

    void init();

    void init_vars(int ids, int ide, int jds, int jde, int kds, int kde);

    void run(CMTime& startTime, CMTime& stopTime);

    void setup_kdtree();

    IDX2 PointCloud_idx_to_ij(uint32_t idx);

    int find_corner_k(Geo3D& loc, IDX2& ij);
};

#endif