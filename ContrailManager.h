#ifndef CONTRAILMANAGER
#define CONTRAILMANAGER

#include <vector>
#include "timekeeping.h"
#include "domain.h"
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
    int plumeModel = 1;
    float maxInitialSegLen = 2500; // Maximum length of a new segment (m)
    int maxContrailAge_s = 12*3600;

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

    bool find_flight_loc(const Flight& flight, const CMTime& time, Geo3D& loc, int& lastWp);

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
    bool advect_loc_RK4(Geo3D& loc, const float duration_s);

    bool wind_at_loc(const Geo3D& loc, float& u, float& v, float& w);

public:
    // Domain
    Domain domain;

    // Projection
    Projection proj;

    // External functions

    void init();

    void run(CMTime& startTime, CMTime& stopTime);
    
};

#endif