#ifndef SEGMENT
#define SEGMENT

#include <vector>

// A structure to define a location in lat, lon, alt coordinates
struct Location {
    float lat;
    float lon;
    float alt;
};

// Flight structure
struct Flight {
    int ID;
    std::vector<Location> wp; // Waypoints
    int wpPassed = 0; // Number of waypoints passed
};

// Contrail segment structure
struct Segment {
    Location back;
    Location front;
    Location centre;

    float length;
    float age; // in seconds

    // Some kind of data
};

#endif