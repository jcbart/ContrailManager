#ifndef FLIGHT_H
#define FLIGHT_H

#include <vector>
#include <string>
#include "timekeeping.h"
#include "mapUtils.h"
#include "FlightInputs.h"

// Flight structure
struct Flight {
    std::string ID = "none"; // ID
    int numWps = 0; // Number of waypoints
    std::vector<Geo3D> wpLocs; // Waypoints locations
    std::vector<CMTime> wpTimes; // Time at each waypoint
    
    float engine_efficiency = 0; // Engine efficiency ()
    float ei_h2o = 0; // Emissions index of water vapour (kg (kg fuel)-1)
    float q_fuel = 0; // Specific combustion heat of fuel (J kg-1)
    float aircraft_mass = 0; // Aircraft mass (kg)
    float wingspan = 0; // Aircraft wingspan (m)
    float true_airspeed = 0; // True airspeed (m s-1)
    float fuel_flow = 0; // Fuel flow (kg s-1)
    float T_exhaust = 0; // Exhaust temperature (K)
    float nvpm_ei_n = 0; // Emissions index of nvPM (# (kg fuel)-1)

    // Returns a FlightInputs object based on current flight attributes
    // Will take a waypoint passed index and a float to interpolate some values between waypoints
    FlightInputs createFlightInputs() const {
        FlightInputs flightInputs;

        flightInputs.engine_efficiency = this->engine_efficiency;
        flightInputs.ei_h2o = this->ei_h2o;
        flightInputs.q_fuel = this->q_fuel;
        flightInputs.aircraft_mass = this->aircraft_mass;
        flightInputs.wingspan = this->wingspan;
        flightInputs.true_airspeed = this->true_airspeed;
        flightInputs.fuel_flow = this->fuel_flow;
        flightInputs.T_exhaust = this->T_exhaust;
        flightInputs.nvpm_ei_n = this->nvpm_ei_n;

        return flightInputs;
    }
};

// Comparator - returns true if the first waypoint of flight A occurs before that of flight B
inline bool flightFirstTimeCompare(Flight& A, Flight& B) {
    return A.wpTimes[0] < B.wpTimes[0];
}

#endif