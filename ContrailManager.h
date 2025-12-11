#ifndef CONTRAILMANAGER
#define CONTRAILMANAGER

#include <vector>
#include "timekeeping.h"
#include "variables.h"
#include "segment.h"
#include "flight.h"
#include "projection.h"
#include "mapUtils.h"

class ContrailManager {
private:
    CMTimeInterval timeStep;
    int timeStep_s;
    CMTime currTime;
    bool firstRunCall = true;
    float maxInitialSegLen = 2500; // Maximum length of a new segment (m)
    int maxContrailAge_s = 12*3600;
    // End indices are one smaller than in WRF because CM does not use a staggered grid
    // Longitude is i/x/u direction
    // Latitude is j/y/v direction
    // Altitude is k/z/w direction
    int ids = 0, ide = 0, jds = 0, jde = 0, kds = 0, kde = 0;
    int lonSize = 0, latSize = 0, altSize = 0;
    bool varsInitd = false;

    // Flight vector
    std::vector<Flight> flights;

    // Contrail segment vector
    std::vector<Segment> segments;

    // Main integration functions

    void setup_on_first_run(CMTime& startTime);

    void create_segments(const CMTime& timeStepStart, const CMTime& timeStepEnd);

    void integrate_plumes(const CMTime& timeStepStart, const CMTime& timeStepEnd);

    void advect_segments(const CMTime& timeStepStart, const CMTime& timeStepEnd);

    // Utility functions

    bool loc_to_ij(const Geo2D& loc, IDX2& ij);

    bool loc_to_ijk(const Geo3D& loc, IDX3& ijk);

    Geo2D ij_to_loc(const IDX2& ij);

    Geo3D ijk_to_loc(const IDX3& ijk);

    bool find_flight_loc(const Flight& flight, const CMTime& time, Geo3D& loc, int lastWpIDX);

    void find_dependent_locs(Segment& seg);

    Geo3D great_circle_interp(const float f, const Geo3D& loc1, const Geo3D& loc2);

    Geo3D great_circle_interp(const CMTime& time, const CMTime& time1, const Geo3D& loc1,
                              const CMTime& time2, const Geo3D& loc2);
    
    bool find_interp(const Geo3D& loc, Interp& interp);

    bool find_interp_points(const Geo3D& loc, Interp& interp);

    bool find_k_inside(const Geo3D& loc, const IDX2& ij, int& k);

    bool find_k_below(const Geo3D& loc, const IDX2& ij, int& k);

    void find_interp_weights(const Geo3D& loc, Interp& interp);

    bool advect_loc(Geo3D& loc, const float duration_s);

public:
    // Meteorological variables (accessible externally)
    Variable2D XLONG; // Longitude (degrees, West is negative)
    Variable2D XLAT; // Latitude (degrees, South is negative)
    Variable3D Z; // Height above sea level at cell centre (m)
    Variable3D Z_AT_W; // Height above sea level at cell interfaces (staggered in z-direction; m)
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

    // Projection
    Projection proj;

    // External functions

    void init();

    void init_vars(int ids, int ide, int jds, int jde, int kds, int kde);

    void run(CMTime& startTime, CMTime& stopTime);
    
};

#endif