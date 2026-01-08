#include <ESMC.h>
#include <iostream>
#include <string>
#include <cmath>
#include <memory>
#include <yaml-cpp/yaml.h>
#include "ContrailManager.h"
#include "timekeeping.h"
#include "domain.h"
#include "segment.h"
#include "flight.h"
#include "projection.h"
#include "mapUtils.h"
#include "plumeModels.h"

void ContrailManager::init() {
    int rc;
    std::string msg;
    rc = ESMC_LogWrite("Initialising Contrail Manager:", ESMC_LOGMSG_INFO);
    
    read_config();
    
    msg = "Contrail Manager internal time step set to " + timeStep.asString();
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);

    msg = "Online coupling: " + std::string(domain.onlineCoupling ? "true" : "false");
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);

    // Determine plume model
    std::string plumeModelStr;
    switch (plumeModelID) {
        case MODEL_ID_BASIC_PLUME: {
            // Set pointer to specialised segment container
            segments = std::unique_ptr<SegmentContainer<SegmentBasicPlume>>(
                new SegmentContainer<SegmentBasicPlume>());
            plumeModelStr = MODEL_STR_BASIC_PLUME;
            break;
        }
        default: {
            std::cerr << "Plume model " << plumeModelID << " not recognised. Stopping.";
            exit(EXIT_FAILURE);
        }
    }
    msg = "Plume model: " + plumeModelStr;
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);

    // After the segments pointer has been set
    segments->maxContrailAge_s = maxContrailAge_s;
    segments->domPtr = &domain;

    // Read flight data etc
    Flight test_flight;
    test_flight.ID = "1";
    CMTime time1 = {2025, 4, 1, 0, 0, 10};
    CMTime time2 = {2025, 4, 1, 0, 2, 0};
    test_flight.wpTimes.push_back(time1);
    test_flight.wpTimes.push_back(time2);
    Geo3D loc1 = {-0.71, 51.73, 10e3};
    Geo3D loc2 = {-1.05, 51.76, 11e3};
    test_flight.wpLocs.push_back(loc1);
    test_flight.wpLocs.push_back(loc2);
    test_flight.numWps = 2;
    flights.push_back(test_flight);
    rc = ESMC_LogWrite("Contrail Manager initialised", ESMC_LOGMSG_INFO);
}

// Read config file
void ContrailManager::read_config() {
    YAML::Node config = YAML::LoadFile("CM-config.yaml");

    int timeStep_s = config["Time step (s)"].as<int>();
    if (timeStep_s <= 0) {
        std::cerr << "Config error: Read time step of " << timeStep_s << " s." << std::endl;
        std::cerr << "Time step must be positive. Stopping." << std::endl;
        exit(EXIT_FAILURE);
    }
    timeStep.set(0, 0, 0, 0, 0, timeStep_s);

    domain.onlineCoupling = config["Online coupling"].as<bool>();

    plumeModelID = config["Plume model"].as<int>();

    maxInitialSegLen = config["Max initial segment length (m)"].as<float>();
    if (maxInitialSegLen <= 0) {
        std::cerr << "Config error: Read maximum initial segment length of " << maxInitialSegLen
                  << " m." << std::endl;
        std::cerr << "Maximum initial segment length must be positive. Stopping." << std::endl;
        exit(EXIT_FAILURE);
    }

    float maxContrailAge_h = config["Max contrail age (h)"].as<float>();
    if (maxContrailAge_h <= 0) {
        std::cerr << "Config error: Read maximum contrail age of " << maxContrailAge_h
                  << " h." << std::endl;
        std::cerr << "Maximum contrail age must be positive. Stopping." << std::endl;
        exit(EXIT_FAILURE);
    }
    maxContrailAge_s = 3600 * maxContrailAge_h;
}

// Integrate between times
void ContrailManager::run(CMTime& startTime, CMTime& stopTime) {
    int rc;
    std::string msg;

    if (firstRunCall) {
        setup_on_first_run(startTime);
        firstRunCall = false;
    }
    
    // Check startTime matches expected time
    if (currTime != startTime) {
        std::cerr << "Error: currTime (" << currTime.asString() << ") does not match "
                  << "integration startTime (" << startTime.asString() << ")" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Check there are a whole number of time steps between startTime and stopTime
    CMTimeInterval timeInterval = stopTime-startTime;
    if (timeInterval.dhms_to_s() % timeStep.dhms_to_s() != 0) {
        std::cerr << "Error: Integration time interval (" << timeInterval.asString()
                  << ") is not an integer multiple of time step ("
                  << timeStep.asString() << ")" << std::endl;
        exit(EXIT_FAILURE);
    }

    msg = "Integrating between " + startTime.asString() + " and "
          + stopTime.asString();
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);

    if (domain.onlineCoupling) {
        // Set all delta variables and contrail ice mass to zero (will be built up again)
        domain.deltaQV.clear_all();
        domain.deltaQI.clear_all();
        domain.deltaNI.clear_all();
        domain.QIcontrail.clear_all();
    }

    while (currTime+timeStep <= stopTime) {
        CMTime timeStepStart = currTime;
        CMTime timeStepEnd = currTime + timeStep;
        /*
        1. Create new segments
        2. Integrate all segment plumes (aggregate vapour delta based on start location,
           mark dead segments)
        3. Dump old/dead segments before advection (aggregate leftover crystals)
        4. Advect all segments (update dependent segments locs and find new length and width)
        5. Increment currTime
        */
        
        // 1. Create segments
        create_segments(timeStepStart, timeStepEnd);

        // 2. Integrate plumes
        segments->integratePlumes(timeStepStart, timeStepEnd);

        // 3. Dump old or dead segments in the same location they were integrated (check if timeStepEnd)
        segments->dump(timeStepEnd);

        // 4. Advect segments
        segments->advectSegments(timeStepStart, timeStepEnd);

        // 5. Increment currTime
        currTime = timeStepEnd;
        msg = "Current time: " + currTime.asString();
        rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
        msg = "Number of live contrail segments: " + std::to_string(segments->getSize());
        rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
    }

    if (domain.onlineCoupling) {
        // Construct QIcontrail field from the live contrail ice mass before ending run
        segments->constructQIcontrail();
    }
}


// Completes the setup required on the first run call (i.e. after getting external data)
void ContrailManager::setup_on_first_run(CMTime& startTime) {
    int rc;
    std::string msg;

    if (!domain.get_varsInitd()) {
        std::cerr << "ContrailManager run called before vars have been initialised. Stopping."
                  << std::endl;
        exit(EXIT_FAILURE);
    }
    if (!domain.proj.isInitd) {
        std::cerr << "ContrailManager run called before projection has been initialised. Stopping."
                  << std::endl;
        exit(EXIT_FAILURE);
    }

    currTime = startTime;
    msg = "Contrail Manager current time set to " + currTime.asString();
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
}

// Create new segments from flights
void ContrailManager::create_segments(const CMTime& timeStepStart, const CMTime& timeStepEnd) {
    int rc;
    std::string msg;

    int num_created = 0;
    for (const Flight& flight : flights) {
        msg = "Creating segments for flight: " + flight.ID;
        rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
        // Find start and end locations of flight in time step
        Geo3D flStartLoc, flEndLoc;
        int lastWpStart, lastWpEnd;
        bool startFound = find_flight_loc(flight, timeStepStart, flStartLoc, lastWpStart);
        if (!startFound) continue;
        bool endFound = find_flight_loc(flight, timeStepEnd, flEndLoc, lastWpEnd);
        if (!endFound) continue;

        // n iterates each leg since the route may be sectioned by waypoints
        for (int n = 0; n <= lastWpEnd - lastWpStart; n++) {
            // Find start and end locs and times for leg
            Geo3D legStartloc, legEndLoc;
            CMTime legStartTime, legEndTime;
            // If first leg
            if (n == 0) {
                legStartloc = flStartLoc;
                legStartTime = timeStepStart;
            }
            else {
                legStartloc = flight.wpLocs[lastWpStart+n];
                legStartTime = flight.wpTimes[lastWpStart+n];
            }
            // If last leg
            if (lastWpStart + n == lastWpEnd) {
                legEndLoc = flEndLoc;
                legEndTime = timeStepEnd;
            }
            else {
                legEndLoc = flight.wpLocs[lastWpStart+n+1];
                legEndTime = flight.wpTimes[lastWpStart+n+1];
            }

            // Create as many segments as needed between
            float distInLeg = great_circle_dist(legStartloc, legEndLoc);
            int numNewSegments = ceil(distInLeg / maxInitialSegLen);
            float segLen = distInLeg / numNewSegments;
            Geo3D backLoc = legStartloc;
            Geo3D frontLoc;
            for (int i = 0; i < numNewSegments; i++) {
                msg = "Segment " + std::to_string(i) + ":";
                rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);

                // Find new front loc
                // Fraction of the total distance where the front of the segment is
                float f_front = (i+1.)/numNewSegments;
                frontLoc = great_circle_interp(f_front, legStartloc, legEndLoc);

                // Use find_interp to find if in grid
                // If any segment location is not in the grid, don't add the segment
                bool inGrid;
                std::vector<IDX3> interpTemp;
                inGrid = domain.find_interp_points(backLoc, interpTemp);
                if (!inGrid) {continue;}
                inGrid = domain.find_interp_points(frontLoc, interpTemp);
                if (!inGrid) {continue;}       

                // Find birth time
                // Fraction of leg duration passed at centre of segment
                float f_centre = (i+0.5)/numNewSegments;
                CMTime birthTime = legStartTime + f_centre * (legEndTime - legStartTime);

                // Add emissions info

                // Add segment to container
                segments->addItem(flight.ID, backLoc, frontLoc, segLen, birthTime);

                // Set back loc for next segment
                backLoc = frontLoc;

                num_created++;
            }
        }
    }
    msg = "Number of segments created: " + std::to_string(num_created);
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
}

// Finds the flight location at the given time with a great circle interpolation between
// neighbouring waypoints
// loc is given the location
// lastWp is given the index of the last waypoint passed (e.g. 0 for 0th waypoint)
// Returns false if flight is before first or after last waypoint at time
bool ContrailManager::find_flight_loc(const Flight& flight, const CMTime& time, Geo3D& loc,
                                      int& lastWp) {
    if (time < flight.wpTimes[0] || time > flight.wpTimes[flight.numWps-1]) {
        // Flight is before first waypoint or after last waypoint
        return false;
    }
    // Else, flight is within waypoint route
    // Find last waypoint passed
    lastWp = 0;
    for (int i = 0; i < flight.numWps-1; i++) {
        if (time >= flight.wpTimes[i] && time < flight.wpTimes[i+1]) {
            lastWp = i;
            break;
        }
    }
    // Flight is between lastWp and lastWp+1
    loc = great_circle_interp(time, flight.wpTimes[lastWp], flight.wpLocs[lastWp],
                              flight.wpTimes[lastWp+1], flight.wpLocs[lastWp+1]);
    return true;
}