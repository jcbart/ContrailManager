#ifndef FLIGHT
#define FLIGHT

#include <vector>
#include "mapUtils.h"

// Flight structure
struct Flight {
    int ID;
    std::vector<Geo3D> wp; // Waypoints
    int wpPassed = 0; // Number of waypoints passed
};

#endif