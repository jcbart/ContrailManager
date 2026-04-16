#ifndef CONTRAILMANAGER_H
#define CONTRAILMANAGER_H

#include <string>
#include <vector>
#include <memory>
#include "CMConfig.h"
#include "PlumeModels.h"
#include "Domain.h"
#include "FlightContainer/FlightContainer.h"
#include "SegmentContainer.h"
#include "Flight.h"

// Forward declarations
struct CMTime;
struct Geo3D;

class ContrailManager {
private:
    CMTime currTime; // Current simulation time
    CMTimeInterval outputInterval; // Time interval between outputs
    CMTime nextOutputTime; // Next time to output

    PlumeModels::Model plumeModel; // Plume model

    bool firstRunCall = true; // Determines whether to call setup_on_first_run

    CMConfig config; // Config read from file

    FlightContainer flights;

    // Contrail segment container (is given a specific type during initialisation)
    SegmentContainerVariant segments;

    // Completes the setup required on the first run call (i.e. after getting external data)
    void setup_on_first_run(const CMTime& startTime);

    // Create new segments from flights (parallelised)
    void create_segments(const CMTime& startTime, const CMTime& stopTime);

public:
    // Pointer to domain
    std::shared_ptr<Domain> domain;

    // Initialise Contrail Manager
    void init();

    // Run Contrail Manager between times
    void run(const CMTime& startTime, const CMTime& stopTime);
    
};

#endif