#ifndef CONTRAILMANAGER_H
#define CONTRAILMANAGER_H

#include <string>
#include <vector>
#include <memory>
#include "Domain.h"
#include "FlightContainer.h"
#include "SegmentContainer.h"
#include "Flight.h"

// Forward declarations
struct CMTime;
struct Geo3D;

class ContrailManager {
private:
    CMTime currTime;

    bool firstRunCall = true; // Determines whether to call setup_on_first_run

    std::string flightDataFilepath;
    int plumeModelID = 0; // ID of the plume model to use
    bool twoWayCoupling = true; // True for two-way coupling (feedback to NWP)
    float maxInitialSegLen = 2500; // Maximum length of a new segment (m)
    float maxContrailAge_s = 12*3600; // Maximum age of a contrail segment (s)
    // Maximum ratio of double-counted water vapour mass in contrail plume to water vapour mass in
    // grid cell ()
    float maxAccumVapRatio = 1e-2;

    FlightContainer flights;

    // Pointer to the contrail segment container (is given a pointer to a
    // SegmentContainer<SegmentType> during initialisation)
    std::unique_ptr<ISegmentContainer> segments;

    void read_config();

    // Completes the setup required on the first run call (i.e. after getting external data)
    void setup_on_first_run(const CMTime& startTime);

    // Create new segments from flights
    void create_segments(const CMTime& startTime, const CMTime& stopTime);

public:
    // Pointer to domain
    std::unique_ptr<Domain> domain;

    // Initialise Contrail Manager
    void init();

    // Run Contrail Manager between times
    void run(const CMTime& startTime, const CMTime& stopTime);
    
};

#endif