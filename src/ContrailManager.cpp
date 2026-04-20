#include <string>
#include <memory>
#include <chrono>
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
#include "Flight.h"
#include "FlightInputs.h"
#include "Projection.h"
#include "map/functions.h"
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
    // Sets variant to specific segment container
    switch (config.plumeModelID) {
        case PlumeModels::COCIP.ID: {
            plumeModel = PlumeModels::COCIP;
#ifdef WITH_COCIP
            segments = SegmentContainer<SegmentCoCiP>{};
            std::visit([](auto& s) {
                s.cocipParams = std::make_shared<Params>();
                s.cocipParams->readYAML();
            }, segments);
#else
            CM_RaiseError("Contrail Manager has not been built with " + plumeModel.name,
                __FILE__, __LINE__);
#endif
            break;
        }
        default: {
            CM_RaiseError(std::format("Plume model {} not recognised", config.plumeModelID),
                __FILE__, __LINE__);
        }
    }
    CM_LogWrite(std::format("Plume model: {}", plumeModel.name));

    // After the segment container has been set
    std::visit([&](auto& s) {
        s.maxContrailAge_s = config.maxContrailAge_s;
        s.maxSegLen = config.maxSegLen;
        s.maxAccumVapRatio = config.maxAccumVapRatio;
    }, segments);

    outputInterval.set(std::chrono::duration<double>(config.outputInterval_s));

    flights.maxInitialSegLen = config.maxInitialSegLen;
    flights.readDatasets(config.flightDatasetPaths);

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

    flights.updateActive(startTime, stopTime);
        
    // 1. Create segments
    create_segments(startTime, stopTime);

    std::visit([&](auto& s) {
        // 2. Evolve plumes
        s.evolvePlumes(startTime, stopTime);

        // 3. Advect segments
        s.advectSegments(startTime, stopTime);

        // 4. Dump old or dead segments in their new location
        s.dump(stopTime);
    }, segments);

    // 5. Update currTime
    currTime = stopTime;

    size_t numSegments = std::visit([](auto& s) -> size_t {
        return s.getSize();
    }, segments);

    CM_LogWrite("Current time: " + currTime.asString());
    CM_LogWrite(std::format("Number of live contrail segments: {}", numSegments));

    if (currTime >= nextOutputTime) {
        // Time to output
        std::visit([&](auto& s) {
            s.save(currTime, plumeModel);
        }, segments);
        nextOutputTime += outputInterval;
    }

    if (domain->twoWayCoupling) {
        // Update deltaQV field
        //domain->find_deltaQV();
        // Construct QIcontrail field from the live contrail ice mass
        std::visit([](auto& s) {
            s.constructQIcontrail();
        }, segments);
    }

    // Check exported fields are valid
    domain->check_valid_exports();

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

    // Set domain-related variables now that domain is initialised
    domain->twoWayCoupling = config.twoWayCoupling;
    std::visit([&](auto& s) {
        s.domain = domain;
    }, segments);

    currTime = startTime;

    nextOutputTime = startTime + outputInterval;

    CM_LogWrite("Contrail Manager current time set to " + currTime.asString());

    if (config.restartRun) {
        // Load segments from file
        std::visit([&](auto& s) {
            s.load(currTime, plumeModel);
        }, segments);
    }
}

void ContrailManager::create_segments(const CMTime& startTime, const CMTime& stopTime) {
    CM_LogWrite("Creating segments for " + std::to_string(flights.active.size())
        + " active flights");

    size_t numBefore = std::visit([](auto& s) -> size_t {
        return s.getSize();
    }, segments);

    // Create segments from each flight using lambda function telling flights what to do with
    // resulting FlightInputs objects
    flights.createSegments(startTime, stopTime, *domain,
        [this](FlightInputs inputs) {
            std::visit([&inputs](auto& s) {
                s.addItem(inputs);
            }, segments);
        }
    );

    size_t numAfter = std::visit([](auto& s) -> size_t {
        return s.getSize();
    }, segments);
    
    CM_LogWrite(std::format("Number of segments created: {}", (numAfter - numBefore)));
}