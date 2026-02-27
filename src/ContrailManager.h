#ifndef CONTRAILMANAGER_H
#define CONTRAILMANAGER_H

#include <vector>
#include <memory>
#include "Domain.h"
#include "SegmentContainer.h"
#include "Flight.h"

// Forward declarations
struct CMTime;
struct Geo3D;

class ContrailManager {
private:
    CMTimeInterval timeStep;
    CMTime currTime;

    bool firstRunCall = true; // Determines whether to call setup_on_first_run

    int plumeModelID = 0; // ID of the plume model to use
    bool twoWayCoupling = true; // True for two-way coupling (feedback to NWP)
    float maxInitialSegLen = 2500; // Maximum length of a new segment (m)
    float maxContrailAge_s = 12*3600; // Maximum age of a contrail segment (s)
    // Maximum ratio of double-counted water vapour mass in contrail plume to water vapour mass in
    // grid cell ()
    float maxAccumVapRatio = 1e-2;

    // Vector of flights loaded in memory
    std::vector<Flight> stagedFlights;

    // Vector of flights currently in their trajectory
    std::vector<Flight> activeFlights;

    // Index of the last flight in stagedFlights added to activeFlights
    // Must start at -1 so that 0 is the first index checked
    size_t lastFlightAdded = -1;

    // Pointer to the contrail segment container (is given a pointer to a
    // SegmentContainer<SegmentType> during initialisation)
    std::unique_ptr<ISegmentContainer> segments;

    void read_config();

    void read_flight_dataset();

    void setup_on_first_run(const CMTime& startTime);

    void update_active_flights(const CMTime& startTime, const CMTime& stopTime);

    void create_segments(const CMTime& timeStepStart, const CMTime& timeStepEnd);

    int find_last_wp(const Flight& flight, const CMTime& time);

    bool find_flight_loc(const Flight& flight, const CMTime& time, Geo3D& loc);

public:
    // Pointer to domain
    std::unique_ptr<IDomain> domain;

    // External functions

    void init();

    void run(CMTime& startTime, CMTime& stopTime);
    
};

#endif