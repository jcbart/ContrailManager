#ifndef CONTRAILMANAGER_H
#define CONTRAILMANAGER_H

#include <string>
#include <vector>
#include <memory>
#include "CMConfig.h"
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

    CMConfig config; // Config read from file

    FlightContainer flights;

    // Pointer to the contrail segment container (is given a pointer to a
    // SegmentContainer<SegmentType> during initialisation)
    std::unique_ptr<ISegmentContainer> segments;

    // Completes the setup required on the first run call (i.e. after getting external data)
    void setup_on_first_run(const CMTime& startTime);

    // Create new segments from flights (parallelised)
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