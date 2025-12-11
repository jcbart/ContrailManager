#ifndef FLIGHT
#define FLIGHT

#include <vector>
#include "timekeeping.h"
#include "mapUtils.h"

// Flight structure
struct Flight {
    int ID = -1;
    std::vector<Geo3D> wpLocs; // Waypoints locations
    std::vector<CMTime> wpTimes; // Time at each waypoint
    int numWps = 0;
    // Number of waypoints passed (wpsPassed = n means flight is between wpLocs[n-1] and wpLocs[n])
    int wpsPassed = 0;
};

#endif