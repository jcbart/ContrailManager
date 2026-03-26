#ifndef WAYPOINT_H
#define WAYPOINT_H

#include "timekeeping.h"
#include "mapTypes.h"

// Flight waypoint structure (holds a time, location, and some aircraft values)
struct Waypoint {
    CMTime time; // Timestamp
    Geo3D loc; // Location
    float aircraft_mass; // Aircraft mass (kg)
    float fuel_flow; // Fuel flow (kg s-1)
    float engine_efficiency; // Engine efficiency ()
    float nvpm_ei_n; // Emissions index of nvPM (# (kg fuel)-1)

    // Empty constructor
    Waypoint() {}

    // Constructor with values
    Waypoint(const CMTime& time, const Geo3D& loc, float aircraft_mass, float fuel_flow,
        float engine_efficiency, float nvpm_ei_n)
        : time(time), loc(loc), aircraft_mass(aircraft_mass), fuel_flow(fuel_flow),
          engine_efficiency(engine_efficiency), nvpm_ei_n(nvpm_ei_n) {}
};

#endif