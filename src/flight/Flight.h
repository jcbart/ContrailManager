#ifndef FLIGHT_H
#define FLIGHT_H

#include <vector>
#include <string>
#include <algorithm>
#include "flight/Aircraft.h"
#include "flight/Waypoint.h"
#include "flight/FlightInputs.h"
#include "domain/Domain.h"
#include "map/functions.h"
#include "CMLog.h"

// Flight structure
struct Flight {
    std::string ID; // ID
    Aircraft aircraft; // Aircraft data
    std::vector<Waypoint> waypoints; // Waypoints

    // Constructor
    Flight(const std::string_view ID, const Aircraft& aircraft,
        const std::vector<Waypoint>& waypoints)
        : ID(ID), aircraft(aircraft), waypoints(waypoints) {}

    size_t numWaypoints() const {
        return waypoints.size();
    }

    // Find the last waypoint passed by the flight at the given time (e.g. 0 for 0th waypoint)
    // If before the 0th waypoint, lastWp = -1
    size_t find_last_wp(const CMTime& time) const {
        if (time < waypoints.front().time) {
            // Flight is before first waypoint
            return -1;
        }
        if (time >= waypoints.back().time) {
            // Flight is after last waypoint
            return numWaypoints() - 1;
        }
        // Else, flight is within waypoint route
        // Iterator pointing to first waypoint after time
        auto it = std::ranges::upper_bound(waypoints, time, {}, &Waypoint::time);
        // Return distance to iterator - 1 (last waypoint passed)
        return std::distance(waypoints.begin(), it) - 1;
    }

    // Finds the flight location at the given time with a great circle interpolation between
    // neighbouring waypoints
    // loc is given the location
    // Returns false if flight is before first or after last waypoint at time
    bool find_loc(const CMTime& time, Geo3D& loc) const {
        size_t lastWp = find_last_wp(time);
        if (lastWp == -1 || lastWp == numWaypoints() - 1) {
            // Flight is before first or after last waypoint
            return false;
        }
        // Flight is between lastWp and lastWp + 1
        loc = map::great_circle_interp(time, waypoints[lastWp], waypoints[lastWp + 1]);
        return true;
    }

    // Create segments from flight
    // For each segment, pass the resulting FlightInputs to `emit`
    template <typename Emit>
    void createSegments(const CMTime& startTime, const CMTime& stopTime, const Domain& domain,
        const float maxInitialSegLen, Emit&& emit
    ) const {
        // Find last waypoint passed at start and end of time interval
        size_t lastWpStart = find_last_wp(startTime);
        size_t lastWpEnd = find_last_wp(stopTime);

        // Iterate through each leg (sectioned by waypoints) between start and end locations
        for (size_t n = lastWpStart; n <= lastWpEnd; n++) {
            // Find start and end locations and times of leg
            Geo3D legStartLoc, legEndLoc;
            CMTime legStartTime, legEndTime;
            // If leg is before first or after last waypoint, cannot find flight loc
            if (n == -1 || n == numWaypoints() - 1) {
                continue;
            }
            // If first leg, start from flight start loc (not wp)
            if (n == lastWpStart) {
                // Safe to ignore return value
                bool startFound = find_loc(startTime, legStartLoc);
                legStartTime = startTime;
            }
            // Else, leg starts at wp
            else {
                legStartLoc = waypoints[n].loc;
                legStartTime = waypoints[n].time;
            }
            // If last leg, end at flight end loc (not wp)
            if (n == lastWpEnd) {
                // Safe to ignore return value
                bool endFound = find_loc(stopTime, legEndLoc);
                legEndTime = stopTime;
            }
            // Else, leg ends at wp
            else {
                legEndLoc = waypoints[n + 1].loc;
                legEndTime = waypoints[n + 1].time;
            }

            // Create as many segments as needed between
            double distInLeg = map::great_circle_dist(legStartLoc, legEndLoc);
            int numNewSegments = ceil(distInLeg / maxInitialSegLen);
            Geo3D backLoc = legStartLoc;
            Geo3D frontLoc;
            for (int i = 0; i < numNewSegments; i++) {
                // Find new front loc
                // Fraction of the total distance where the front of the segment is
                double f_leg_front = (i + 1.)/numNewSegments;
                frontLoc = map::great_circle_interp(f_leg_front, legStartLoc, legEndLoc);

                // Find if interpolation is possible
                // If interpolation is not possible for any segment location, don't add the segment
                if (!(domain.can_do_interp(backLoc) && domain.can_do_interp(frontLoc))) {
                    continue;
                }

                // Find birth time
                // Fraction of leg duration passed at centre of segment
                double f_leg_centre = (i + 0.5) / numNewSegments;
                CMTime birthTime = legStartTime + f_leg_centre * (legEndTime - legStartTime);

                // Fraction of duration between last and next waypoints passed at centre of segment
                double f_waypoint = (
                    (birthTime - waypoints[n].time)
                    / (waypoints[n + 1].time - waypoints[n].time)
                );

                FlightInputs inputs(ID, birthTime, backLoc, frontLoc);
                inputs.emissions = createFlightEmissions(n, f_waypoint);

                // Emit (add segment to container) with FlightInputs object
                emit(inputs);

                // Set back loc for next segment
                backLoc = frontLoc;
            }
        }
    }

    // Returns a FlightEmissions object based on current flight attributes
    // Current flight attributes are determined using the last waypoint index (lastWp) and
    // the fraction of duration between last and next waypoints passed at centre of segment
    // (f_waypoint)
    FlightEmissions createFlightEmissions(const size_t lastWp, const double f_waypoint) const {
        const Waypoint& lastWaypoint = waypoints[lastWp];
        const Waypoint& nextWaypoint = waypoints[lastWp + 1];

        FlightEmissions emissions;

        // Interpolate
        emissions.engine_efficiency = (
            f_waypoint * lastWaypoint.engine_efficiency
            + (1 - f_waypoint) * nextWaypoint.engine_efficiency
        );
        emissions.aircraft_mass = (
            f_waypoint * lastWaypoint.aircraft_mass
            + (1 - f_waypoint) * nextWaypoint.aircraft_mass
        );
        emissions.fuel_flow = (
            f_waypoint * lastWaypoint.fuel_flow
            + (1 - f_waypoint) * nextWaypoint.fuel_flow
        );
        emissions.nvpm_ei_n = (
            f_waypoint * lastWaypoint.nvpm_ei_n
            + (1 - f_waypoint) * nextWaypoint.nvpm_ei_n
        );

        emissions.wingspan = aircraft.wingspan;
        emissions.ei_h2o = aircraft.ei_h2o;
        emissions.q_fuel = aircraft.q_fuel;

        // Caluclate speed
        emissions.true_airspeed = (
            map::great_circle_dist(lastWaypoint.loc, nextWaypoint.loc)
            / (nextWaypoint.time - lastWaypoint.time).to_s()
        );

        return emissions;
    }
};

#endif