#include <string>
#include <memory>
#include <chrono>
#include <algorithm>
#include <format>
#ifdef WITH_COCIP
#include <CoCiP++/params.h>
#endif
#include "ContrailManager.h"
#include "timekeeping.h"
#include "Domain.h"
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
    
    config.read();

    CM_LogWrite("Online coupling: " + std::string(config.twoWayCoupling ? "true" : "false"));

    // Determine plume model
    // Sets pointer to specialised segment container

    std::string plumeModelStr;
    switch (config.plumeModelID) {
        case MODEL_ID_COCIP: {
            plumeModelStr = MODEL_STR_COCIP;
#ifdef WITH_COCIP
            segments = std::unique_ptr<SegmentContainer<SegmentCoCiP>>(
                new SegmentContainer<SegmentCoCiP>());
            segments->cocipParams = std::shared_ptr<Params>(new Params);
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
    segments->domPtr = domain.get();

    currTime = startTime;

    CM_LogWrite("Contrail Manager current time set to " + currTime.asString());
}

void ContrailManager::create_segments(const CMTime& startTime, const CMTime& stopTime) {
    CM_LogWrite("Creating segments for " + std::to_string(flights.active.size())
        + " active flights");
    
    size_t num_created = 0;
    for (const Flight& flight : flights.active) {
        CM_LogWrite("Creating segments for flight: " + flight.ID);

        // Find last waypoint passed at start and end of time interval
        size_t lastWpStart = flight.find_last_wp(startTime);
        size_t lastWpEnd = flight.find_last_wp(stopTime);

        // Iterate through each leg (sectioned by waypoints) between start and end locations
        for (size_t n = lastWpStart; n <= lastWpEnd; n++) {
            // Find start and end waypoints of leg
            Waypoint legStart, legEnd;
            // If leg is before first or after last waypoint, cannot find flight loc
            if (n == -1 || n == flight.numWaypoints() - 1) {
                continue;
            }
            // If first leg, start from flight start loc (not wp)
            if (n == lastWpStart) {
                // Safe to ignore return value
                bool startFound = flight.find_loc(startTime, legStart.loc);
                legStart.time = startTime;
            }
            // Else, leg starts at wp
            else {
                legStart.loc = flight.waypoints[n].loc;
                legStart.time = flight.waypoints[n].time;
            }
            // If last leg, end at flight end loc (not wp)
            if (n == lastWpEnd) {
                // Safe to ignore return value
                bool endFound = flight.find_loc(stopTime, legEnd.loc);
                legEnd.time = stopTime;
            }
            // Else, leg ends at wp
            else {
                legEnd.loc = flight.waypoints[n+1].loc;
                legEnd.time = flight.waypoints[n+1].time;
            }

            // Create as many segments as needed between
            double distInLeg = great_circle_dist(legStart.loc, legEnd.loc);
            int numNewSegments = ceil(distInLeg / config.maxInitialSegLen);
            double segLen = distInLeg / numNewSegments;
            Geo3D backLoc = legStart.loc;
            Geo3D frontLoc;
            for (int i = 0; i < numNewSegments; i++) {
                CM_LogWrite(std::format("Segment {}:", i));

                // Find new front loc
                // Fraction of the total distance where the front of the segment is
                double f_front = (i + 1.)/numNewSegments;
                frontLoc = great_circle_interp(f_front, legStart.loc, legEnd.loc);

                // Find if interpolation is possible
                // If interpolation is not possible for any segment location, don't add the segment
                bool canDoInterp;
                canDoInterp = domain->can_do_interp(backLoc);
                if (!canDoInterp) { continue; }
                canDoInterp = domain->can_do_interp(frontLoc);
                if (!canDoInterp) { continue; }

                // Find birth time
                // Fraction of leg duration passed at centre of segment
                double f_centre = (i + 0.5) / numNewSegments;
                CMTime birthTime = legStart.time + f_centre * (legEnd.time - legStart.time);

                // Add emissions info
                FlightInputs flightInputs = flight.createFlightInputs(legStart, legEnd, f_centre);

                // Add segment to container
                segments->addItem(flight.ID, birthTime, flightInputs, backLoc, frontLoc, segLen);

                // Set back loc for next segment
                backLoc = frontLoc;

                num_created++;
            }
        }
    }
    CM_LogWrite(std::format("Number of segments created: {}", num_created));
}