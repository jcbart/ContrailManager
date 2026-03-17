#ifndef FLIGHT_H
#define FLIGHT_H

#include <vector>
#include <string>
#include "Waypoint.h"
#include "FlightInputs.h"
#include "mapFunctions.h"

// Flight structure
struct Flight {
    std::string ID; // ID
    std::vector<Waypoint> waypoints; // Waypoints
    /*
    float engine_efficiency = 0; // Engine efficiency ()
    float ei_h2o = 0; // Emissions index of water vapour (kg (kg fuel)-1)
    float q_fuel = 0; // Specific combustion heat of fuel (J kg-1)
    float aircraft_mass = 0; // Aircraft mass (kg)
    float wingspan = 0; // Aircraft wingspan (m)
    float true_airspeed = 0; // True airspeed (m s-1)
    float fuel_flow = 0; // Fuel flow (kg s-1)
    float T_exhaust = 0; // Exhaust temperature (K)
    float nvpm_ei_n = 0; // Emissions index of nvPM (# (kg fuel)-1)
    */
    float engine_efficiency = 0.3;
    float ei_h2o = 1.25;
    float q_fuel = 43.15e6;
    float aircraft_mass = 70e3;
    float wingspan = 34;
    float fuel_flow = 0.7;
    float T_exhaust = 600;
    float nvpm_ei_n = 1e15;

    // Constructor
    Flight(const std::string_view ID) : ID(ID) {}

    size_t numWaypoints() const {
        return waypoints.size();
    }

    // Returns a FlightInputs object based on current flight attributes
    // Uses two waypoints (may not both be in Flight::waypoints) and fraction
    // travelled between that and the next waypoint to calculate and interpolate some values
    FlightInputs createFlightInputs(const Waypoint& legStart, const Waypoint& legEnd, const double f) const {
        FlightInputs flightInputs;

        flightInputs.engine_efficiency = this->engine_efficiency;
        flightInputs.ei_h2o = this->ei_h2o;
        flightInputs.q_fuel = this->q_fuel;
        flightInputs.aircraft_mass = this->aircraft_mass;
        flightInputs.wingspan = this->wingspan;
        flightInputs.fuel_flow = this->fuel_flow;
        flightInputs.T_exhaust = this->T_exhaust;
        flightInputs.nvpm_ei_n = this->nvpm_ei_n;

        flightInputs.true_airspeed = (
            great_circle_dist(legStart.loc, legEnd.loc)
            / (legEnd.time - legStart.time).to_s()
        );

        return flightInputs;
    }
};

#endif