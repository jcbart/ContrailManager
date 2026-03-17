#ifndef WAYPOINT_H
#define WAYPOINT_H

#include "timekeeping.h"
#include "mapTypes.h"

// Waypoint structure (holds a time and location)
struct Waypoint {
    CMTime time; // Waypoint timestamp
    Geo3D loc; // Waypoint location

    // Empty constructor
    Waypoint() {}

    // Constructor with values
    Waypoint(const CMTime& time, const Geo3D& loc) : time(time), loc(loc) {}
};

#endif