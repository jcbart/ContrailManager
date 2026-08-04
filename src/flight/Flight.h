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

    // Find the next waypoint to be passed by the flight at the given time (e.g. 0 for 0th waypoint)
    // If after the final waypoint (numWaypoints() - 1), returns numWaypoints()
    size_t find_next_wp(const CMTime& time) const {
        // Iterator pointing to first waypoint after time
        auto it = std::ranges::upper_bound(waypoints, time, {}, &Waypoint::time);
        // Return distance to iterator
        return std::distance(waypoints.begin(), it);
    }

    // Finds the flight location at the given time with a great circle interpolation between
    // neighbouring waypoints
    // loc is given the location (with geopotential height)
    // Returns false if flight is before first or after last waypoint at time or if either the
    // last or next waypoint is out of bounds
    bool find_loc(const CMTime& time, const Domain& domain, Geo3D& loc) const {
        size_t idx = find_next_wp(time);
        if (idx == 0 || idx == numWaypoints()) {
            // Flight is before first or after last waypoint
            return false;
        }
        // Flight is between idx - 1 and idx
        Waypoint lastWp = waypoints[idx - 1];
        Waypoint nextWp = waypoints[idx];
        // Convert waypoints to geopotential height
        if (!(domain.pres_alt_to_geopt_ht(lastWp.loc)
            && domain.pres_alt_to_geopt_ht(nextWp.loc))) {
            return false;
        }
        loc = map::great_circle_interp(time, lastWp, nextWp);
        return true;
    }

    // Create segments from flight
    // For each segment, pass the resulting FlightInputs to `emit`
    template <typename Emit>
    void createSegments(const CMTime& startTime, const CMTime& stopTime, const Domain& domain,
        const float maxInitialSegLen, Emit&& emit
    ) const {
        // Find next waypoint at start and end of time interval
        size_t nextWpStart = find_next_wp(startTime);
        size_t nextWpEnd = find_next_wp(stopTime);

        // Iterate through each leg (sectioned by waypoints) between start and end locations
        for (size_t n = nextWpStart; n <= nextWpEnd; n++) {
            // Find start and end locations and times of leg
            Geo3D legStartLoc, legEndLoc;
            CMTime legStartTime, legEndTime;
            // If leg is before first or after last waypoint, cannot find flight loc
            if (n == 0 || n == numWaypoints()) {
                continue;
            }
            // If first leg, start from flight start loc (not wp)
            if (n == nextWpStart) {
                // Continue to next leg if start not in bounds
                if (!find_loc(startTime, domain, legStartLoc)) {
                    continue;
                }
                legStartTime = startTime;
            }
            // Else, leg starts at wp
            else {
                legStartLoc = waypoints[n - 1].loc;
                // Continue to next leg if waypoint not in bounds
                if (!domain.pres_alt_to_geopt_ht(legStartLoc)) {
                    continue;
                }
                legStartTime = waypoints[n - 1].time;
            }
            // If last leg, end at flight end loc (not wp)
            if (n == nextWpEnd) {
                // Continue to next leg if end not in bounds
                if (!find_loc(stopTime, domain, legEndLoc)) {
                    continue;
                }
                legEndTime = stopTime;
            }
            // Else, leg ends at wp
            else {
                legEndLoc = waypoints[n].loc;
                // Continue to next leg if waypoint not in bounds
                if (!domain.pres_alt_to_geopt_ht(legEndLoc)) {
                    continue;
                }
                legEndTime = waypoints[n].time;
            }

            // Create as many segments as needed between
            double distInLeg = map::great_circle_dist(legStartLoc, legEndLoc);
            double airspeedInLeg = distInLeg / (legEndTime - legStartTime).to_s();
            int numNewSegments = ceil(distInLeg / maxInitialSegLen);
            Geo3D backLoc = legStartLoc;
            Geo3D frontLoc;
            for (int i = 0; i < numNewSegments; i++) {
                // Find new front loc
                // Fraction of the total distance where the front of the segment is
                double f_leg_front = (i + 1.) / numNewSegments;
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
                    (birthTime - waypoints[n - 1].time)
                    / (waypoints[n].time - waypoints[n - 1].time)
                );

                FlightInputs inputs(ID, birthTime, backLoc, frontLoc);
                inputs.emissions = createFlightEmissions(n, f_waypoint, airspeedInLeg);

                // Emit (add segment to container) with FlightInputs object
                emit(inputs);

                // Set back loc for next segment
                backLoc = frontLoc;
            }
        }
    }

    // Returns a FlightEmissions object based on current flight attributes
    // Current flight attributes are determined using the next waypoint index (nextWp) and
    // the fraction of duration between last and next waypoints passed at centre of segment
    // (f_waypoint)
    FlightEmissions createFlightEmissions(const size_t nextWp, const double f_waypoint,
        const double true_airspeed
    ) const {
        const Waypoint& lastWaypoint = waypoints[nextWp - 1];
        const Waypoint& nextWaypoint = waypoints[nextWp];

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

        emissions.true_airspeed = true_airspeed;

        return emissions;
    }
};

#endif