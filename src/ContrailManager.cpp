#include <string>
#include <memory>
#include <chrono>
//#include <algorithm>
#include <format>
#include <omp.h>
#ifdef WITH_COCIP
#include <CoCiP++/params.h>
#endif
#include "ContrailManager.h"
#include "timekeeping.h"
#include "Domain.h"
#include "PlumeModels.h"
#include "SegmentContainer.h"
#include "Segment.h"
#include "SegmentCoCiP.h"
#include "Flight.h"
#include "FlightInputs.h"
#include "Projection.h"
#include "mapFunctions.h"
#include "CMLog.h"

void ContrailManager::init() {
    CM_LogWrite("Initialising Contrail Manager:");

    // Find number of threads (using parallel region - more reliable)
    int num_threads = 0;
    #pragma omp parallel
    {
        #pragma omp master
        num_threads = omp_get_num_threads();
    }
    CM_LogWrite(std::format("Number of threads: {}", num_threads));
    
    config.read();

    CM_LogWrite("Online coupling: " + std::string(config.twoWayCoupling ? "true" : "false"));

    // Determine plume model
    // Sets pointer to specialised segment container

    std::string plumeModelStr;
    switch (config.plumeModelID) {
        case PlumeModels::MODEL_ID_COCIP: {
            plumeModelStr = PlumeModels::MODEL_STR_COCIP;
#ifdef WITH_COCIP
            segments = std::make_unique<SegmentContainer<SegmentCoCiP>>();
            segments->cocipParams = std::make_shared<Params>();
            segments->cocipParams->readYAML();
#else
            
            CM_RaiseError("Contrail Manager has not been built with " + plumeModelStr,
                __FILE__, __LINE__);
#endif
            break;
        }
        default: {
            CM_RaiseError(std::format("Plume model {} not recognised", config.plumeModelID),
                __FILE__, __LINE__);
        }
    }
    CM_LogWrite("Plume model: " + plumeModelStr);

    // After the segments pointer has been set
    segments->maxContrailAge_s = config.maxContrailAge_s;
    segments->maxAccumVapRatio = config.maxAccumVapRatio;

    flights.maxInitialSegLen = config.maxInitialSegLen;
    flights.read_datasets(config.flightDatasetPaths);

    CM_LogWrite("Contrail Manager initialised");
}

void ContrailManager::run(const CMTime& startTime, const CMTime& stopTime) {
    // Current time at start of run
    std::chrono::steady_clock::time_point computeTimeStart = std::chrono::steady_clock::now();

    if (firstRunCall) {
        setup_on_first_run(startTime);
        firstRunCall = false;
    }
    
    // Check startTime matches expected time
    if (currTime != startTime) {
        CM_RaiseError("currTime (" + currTime.asString() + ") does not match "
            + "run startTime (" + startTime.asString() + ")", __FILE__, __LINE__);
    }

    CM_LogWrite("Running between " + startTime.asString() + " and " + stopTime.asString());

    if (domain->twoWayCoupling) {
        // Save QV and set all delta variables and contrail ice mass to zero
        // (will be built up again)
        //domain->save_QV();
        domain->deltaQV.clear_all();
        domain->deltaQI.clear_all();
        domain->deltaNI.clear_all();
        domain->QIcontrail.clear_all();
    }

    flights.update_active(startTime, stopTime);
        
    // 1. Create segments
    create_segments(startTime, stopTime);

    // 2. Evolve plumes
    segments->evolvePlumes(startTime, stopTime);

    // 3. Advect segments
    segments->advectSegments(startTime, stopTime);

    // 4. Dump old or dead segments in their new location
    segments->dump(stopTime);

    // 5. Update currTime
    currTime = stopTime;
    CM_LogWrite("Current time: " + currTime.asString());
    CM_LogWrite(std::format("Number of live contrail segments: {}", segments->getSize()));

    if (domain->twoWayCoupling) {
        // Update deltaQV field
        //domain->find_deltaQV();
        // Construct QIcontrail field from the live contrail ice mass
        segments->constructQIcontrail();
    }

    // Current time at end of run
    std::chrono::steady_clock::time_point computeTimeEnd = std::chrono::steady_clock::now();

    // Elapsed time in run (seconds)
    std::chrono::duration<double> computeTime = computeTimeEnd - computeTimeStart;

    CM_LogWrite(std::format("Computation time for coupling interval: {:.3f} s", computeTime.count()));
}

void ContrailManager::setup_on_first_run(const CMTime& startTime) {
    if (domain == nullptr) {
        CM_RaiseError("ContrailManager run called before domain has been initialised",
            __FILE__, __LINE__);
    }

    domain->twoWayCoupling = config.twoWayCoupling;
    segments->domain = domain;

    currTime = startTime;

    CM_LogWrite("Contrail Manager current time set to " + currTime.asString());
}

void ContrailManager::create_segments(const CMTime& startTime, const CMTime& stopTime) {
    CM_LogWrite("Creating segments for " + std::to_string(flights.active.size())
        + " active flights");

    size_t numBefore = segments->getSize();

    // Create segments from each flight using lambda function telling flights what to do with
    // resulting FlightInputs objects
    flights.create_segments(startTime, stopTime, *domain,
        [this](FlightInputs inputs) {
            segments->addItem(inputs);
        }
    );

    size_t numAfter = segments->getSize();
    
    CM_LogWrite(std::format("Number of segments created: {}", (numAfter - numBefore)));
}