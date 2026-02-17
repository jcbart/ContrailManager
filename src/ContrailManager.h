#ifndef CONTRAILMANAGER_H
#define CONTRAILMANAGER_H

#include <vector>
#include <memory>
#include "timekeeping.h"
#include "Domain.h"
#include "SegmentContainer.h"
#include "Segment.h"
#include "Flight.h"
#include "Projection.h"
#include "mapUtils.h"

class ContrailManager {
private:
    CMTimeInterval timeStep;
    CMTime currTime;

    bool firstRunCall = true; // Determines whether to call setup_on_first_run

    int plumeModelID = 0;

    float maxInitialSegLen = 2500; // Maximum length of a new segment (m)
    float maxContrailAge_s = 12*3600; // Maximum age of a contrail segment (s)
    // Maximum ratio of double-counted water vapour mass in contrail plume to water vapour mass in
    // grid cell ()
    float maxAccumVapRatio = 1e-2;

    // Flight vector
    std::vector<Flight> flights;

    // Pointer to the contrail segment container (is given a pointer to a
    // SegmentContainer<SegmentType> during initialisation)
    std::unique_ptr<ISegmentContainer> segments;

    void read_config();

    void setup_on_first_run(CMTime& startTime);

    void create_segments(const CMTime& timeStepStart, const CMTime& timeStepEnd);

    int find_last_wp(const Flight& flight, const CMTime& time);

    bool find_flight_loc(const Flight& flight, const CMTime& time, Geo3D& loc);

public:
    // Domain
    Domain domain;

    // External functions

    void init();

    void run(CMTime& startTime, CMTime& stopTime);
    
};

#endif