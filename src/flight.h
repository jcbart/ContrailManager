#ifndef FLIGHT
#define FLIGHT

#include <vector>
#include <string>
#include "timekeeping.h"
#include "mapUtils.h"

// Flight structure
struct Flight {
    std::string ID = "none";
    std::vector<Geo3D> wpLocs; // Waypoints locations
    std::vector<CMTime> wpTimes; // Time at each waypoint
    int numWps = 0;
    // Number of waypoints passed (wpsPassed = n means flight is between wpLocs[n-1] and wpLocs[n])
    int wpsPassed = 0;
};

#endif