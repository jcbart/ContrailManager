#include <string>
#include <memory>
#include <chrono>
#include <algorithm>
#include <yaml-cpp/yaml.h>
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
#include "mapUtils.h"
#include "CMLog.h"

void ContrailManager::init() {
    CM_LogWrite("Initialising Contrail Manager:");
    
    read_config();

    CM_LogWrite("Online coupling: " + std::string(twoWayCoupling ? "true" : "false"));

    // Determine plume model
    // Sets pointer to specialised segment container

    std::string plumeModelStr;
    switch (plumeModelID) {
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
            CM_RaiseError("Plume model " + std::to_string(plumeModelID) + " not recognised",
                __FILE__, __LINE__);
        }
    }
    CM_LogWrite("Plume model: " + plumeModelStr);

    // After the segments pointer has been set
    segments->maxContrailAge_s = maxContrailAge_s;
    segments->maxAccumVapRatio = maxAccumVapRatio;

    read_flight_dataset();

    CM_LogWrite("Contrail Manager initialised");
}

// Read config file
void ContrailManager::read_config() {
    YAML::Node config = YAML::LoadFile("CM-config.yaml");

    twoWayCoupling = config["Two-way coupling"].as<bool>();

    plumeModelID = config["Plume model"].as<int>();

    maxInitialSegLen = config["Max initial segment length (m)"].as<float>();
    if (maxInitialSegLen <= 0) {
        CM_RaiseError("Config error: Read maximum initial segment length of "
            + std::to_string(maxInitialSegLen)
            + " m. Maximum initial segment length must be positive.", __FILE__, __LINE__);
    }

    float maxContrailAge_h = config["Max contrail age (h)"].as<float>();
    if (maxContrailAge_h <= 0) {
        CM_RaiseError("Config error: Read maximum contrail age of "
            + std::to_string(maxContrailAge_h)
            + " h. Maximum contrail age must be positive.", __FILE__, __LINE__);
    }
    maxContrailAge_s = 3600 * maxContrailAge_h;

    maxAccumVapRatio = config["Max accumulated vapour ratio ()"].as<float>();
    if (maxAccumVapRatio <= 0) {
        CM_RaiseError("Config error: Read maximum accumulated vapour ratio of "
            + std::to_string(maxAccumVapRatio)
            + ". Maximum accumulated vapour ratio must be positive.", __FILE__, __LINE__);
    }
}

void ContrailManager::read_flight_dataset() {
    // Read flight data etc
    Flight test_flight;
    test_flight.ID = "1";
    CMTime time1 = {2025, 4, 1, 6, 0, 0};
    CMTime time2 = {2025, 4, 1, 6, 3, 0};
    test_flight.wpTimes.push_back(time1);
    test_flight.wpTimes.push_back(time2);
    Geo3D loc1 = {-9.7, 52.1, 10500};
    Geo3D loc2 = {-9.1, 52.1, 10500};
    test_flight.wpLocs.push_back(loc1);
    test_flight.wpLocs.push_back(loc2);
    test_flight.numWps = 2;
    test_flight.engine_efficiency = 0.3;
    test_flight.ei_h2o = 1.25;
    test_flight.q_fuel = 43.15e6;
    test_flight.aircraft_mass = 70e3;
    test_flight.wingspan = 34;
    test_flight.true_airspeed = 250;
    test_flight.fuel_flow = 0.7;
    test_flight.T_exhaust = 600;
    test_flight.nvpm_ei_n = 1e15;
    stagedFlights.push_back(test_flight);

    // Sort stagedFlights by first waypoint time
    std::sort(
        stagedFlights.begin(),
        stagedFlights.end(),
        [](const Flight& A, const Flight& B) {
            return A.wpTimes[0] < B.wpTimes[0];
        }
    );
}

// Integrate between times
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
            + "integration startTime (" + startTime.asString() + ")", __FILE__, __LINE__);
    }

    CM_LogWrite("Integrating between " + startTime.asString() + " and " + stopTime.asString());

    if (domain->twoWayCoupling) {
        // Save QV and set all delta variables and contrail ice mass to zero
        // (will be built up again)
        //domain->save_QV();
        domain->deltaQV.clear_all();
        domain->deltaQI.clear_all();
        domain->deltaNI.clear_all();
        domain->QIcontrail.clear_all();
    }

    update_active_flights(startTime, stopTime);

    /*
    if (readFlightsInChunks) {
        while (lastFlightAdded == stagedFlights.size()) {
            // Read new chunk into stagedFlights
            // Reset lastFlightAdded
            lastFlightAdded = -1;
            // Update active flights again
            update_active_flights(startTime, stopTime);
        }
    }
    */
        
    // 1. Create segments
    create_segments(startTime, stopTime);

    // 2. Integrate plumes
    segments->integratePlumes(startTime, stopTime);

    // 3. Advect segments
    segments->advectSegments(startTime, stopTime);

    // 4. Dump old or dead segments in their new location
    segments->dump(stopTime);

    // 5. Update currTime
    currTime = stopTime;
    CM_LogWrite("Current time: " + currTime.asString());
    CM_LogWrite("Number of live contrail segments: " + std::to_string(segments->getSize()));

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

    CM_LogWrite("Computation time for coupling interval: "
        + std::to_string(computeTime.count()) + " s");
}


// Completes the setup required on the first run call (i.e. after getting external data)
void ContrailManager::setup_on_first_run(const CMTime& startTime) {
    if (domain == nullptr) {
        CM_RaiseError("ContrailManager run called before domain has been initialised",
            __FILE__, __LINE__);
    }

    domain->twoWayCoupling = twoWayCoupling;
    segments->domPtr = domain.get();

    currTime = startTime;

    CM_LogWrite("Contrail Manager current time set to " + currTime.asString());
}

void ContrailManager::update_active_flights(const CMTime& startTime, const CMTime& stopTime) {
    // Remove flights whose last waypoint is before startTime
    std::erase_if(
        activeFlights,
        [startTime](const Flight& flight) {
            return flight.wpTimes[flight.numWps-1] <= startTime;
        }
    );

    // Add flights whose first waypoint is between startTime and stopTime
    for (size_t i = lastFlightAdded + 1; i < stagedFlights.size(); i++) {
        // Add if wanted
        if ((stagedFlights[i].wpTimes[0] >= startTime)
            && (stagedFlights[i].wpTimes[0] < stopTime)) {
            
            activeFlights.push_back(stagedFlights[i]);
            // Update last flight added (so far)
            lastFlightAdded = i;
        }
        // Break if this (and thus everything after) is not wanted
        else {
            break;
        }
    }
}

// Create new segments from flights
void ContrailManager::create_segments(const CMTime& startTime, const CMTime& stopTime) {
    CM_LogWrite("Creating segments for " + std::to_string(activeFlights.size())
        + " active flights");
    
    size_t num_created = 0;
    for (const Flight& flight : activeFlights) {
        CM_LogWrite("Creating segments for flight: " + flight.ID);

        // Find last waypoint passed at start and end of time interval
        int lastWpStart = find_last_wp(flight, startTime);
        int lastWpEnd = find_last_wp(flight, stopTime);

        // Iterate through each leg (sectioned by waypoints) between start and end locations
        for (int n = lastWpStart; n <= lastWpEnd; n++) {
            // Find start and end locs and times for leg
            Geo3D legStartloc, legEndLoc;
            CMTime legStartTime, legEndTime;
            // If leg is before first or after last waypoint, cannot find flight loc
            if (n == -1 || n == flight.numWps-1) {
                continue;
            }
            // If first leg, start from flight start loc (not wp)
            if (n == lastWpStart) {
                // Safe to ignore return value
                bool startFound = find_flight_loc(flight, startTime, legStartloc);
                legStartTime = startTime;
            }
            // Else, leg starts at wp
            else {
                legStartloc = flight.wpLocs[n];
                legStartTime = flight.wpTimes[n];
            }
            // If last leg, end at flight end loc (not wp)
            if (n == lastWpEnd) {
                // Safe to ignore return value
                bool endFound = find_flight_loc(flight, stopTime, legEndLoc);
                legEndTime = stopTime;
            }
            // Else, leg ends at wp
            else {
                legEndLoc = flight.wpLocs[n+1];
                legEndTime = flight.wpTimes[n+1];
            }

            // Create as many segments as needed between
            double distInLeg = great_circle_dist(legStartloc, legEndLoc);
            int numNewSegments = ceil(distInLeg / maxInitialSegLen);
            double segLen = distInLeg / numNewSegments;
            Geo3D backLoc = legStartloc;
            Geo3D frontLoc;
            for (int i = 0; i < numNewSegments; i++) {
                CM_LogWrite("Segment " + std::to_string(i) + ":");

                // Find new front loc
                // Fraction of the total distance where the front of the segment is
                double f_front = (i+1.)/numNewSegments;
                frontLoc = great_circle_interp(f_front, legStartloc, legEndLoc);

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
                CMTime birthTime = legStartTime + f_centre * (legEndTime - legStartTime);

                // Add emissions info
                FlightInputs flightInputs = flight.createFlightInputs();

                // Add segment to container
                segments->addItem(flight.ID, birthTime, flightInputs, backLoc, frontLoc, segLen);

                // Set back loc for next segment
                backLoc = frontLoc;

                num_created++;
            }
        }
    }
    CM_LogWrite("Number of segments created: " + std::to_string(num_created));
}

// Find the last waypoint passed by the flight at time (e.g. 0 for 0th waypoint)
// If before the 0th waypoint, lastWp = -1
int ContrailManager::find_last_wp(const Flight& flight, const CMTime& time) {
    int lastWp;
    if (time < flight.wpTimes[0]) {
        // Flight is before first waypoint
        lastWp = -1;
    }
    else if (time >= flight.wpTimes[flight.numWps-1]) {
        // Flight is after last waypoint
        lastWp = flight.numWps-1;
    }
    // Else, flight is within waypoint route
    // Find last waypoint passed
    else {
        for (int i = 0; i < flight.numWps-1; i++) {
            if (time < flight.wpTimes[i+1]) {
                lastWp = i;
                break;
            }
        }
    }
    return lastWp;
}

// Finds the flight location at the given time with a great circle interpolation between
// neighbouring waypoints
// loc is given the location
// Returns false if flight is before first or after last waypoint at time
bool ContrailManager::find_flight_loc(const Flight& flight, const CMTime& time, Geo3D& loc) {
    int lastWp = find_last_wp(flight, time);
    if (lastWp == -1 || lastWp == flight.numWps-1) {
        // Flight is before first or after last waypoint
        return false;
    }
    // Flight is between lastWp and lastWp+1
    loc = great_circle_interp(time, flight.wpTimes[lastWp], flight.wpLocs[lastWp],
                              flight.wpTimes[lastWp+1], flight.wpLocs[lastWp+1]);
    return true;
}