#ifndef FLIGHTINPUTS_H
#define FLIGHTINPUTS_H

#include <string>
#include <string_view>
#include "timekeeping.h"
#include "mapTypes.h"

// Flight emission-related inputs to contrail segment
struct FlightEmissions {
    float engine_efficiency = 0; // Engine efficiency ()
    float ei_h2o = 0; // Emissions index of water vapour (kg (kg fuel)-1)
    float q_fuel = 0; // Specific combustion heat of fuel (J kg-1)
    float aircraft_mass = 0; // Aircraft mass (kg)
    float wingspan = 0; // Aircraft wingspan (m)
    float true_airspeed = 0; // True airspeed (m s-1)
    float fuel_flow = 0; // Fuel flow (kg s-1)
    float nvpm_ei_n = 0; // Emissions index of nvPM (# (kg fuel)-1)
};

// Struct containing all flight inputs used to create contrail segment
struct FlightInputs {
    std::string ID; // Flight ID

    CMTime birthTime; // Segment birth time
    Geo3D back; // Location of back (first point created) of segment
    Geo3D front; // Location of front (last point created) of segment

    FlightEmissions emissions; // Flight emissions

    // Empty constructor
    FlightInputs() {}

    // Constructor with values (except emissions)
    FlightInputs(std::string_view ID, CMTime& birthTime, Geo3D& back, Geo3D front)
        : ID(ID), birthTime(birthTime), back(back), front(front) {}
};

#endif