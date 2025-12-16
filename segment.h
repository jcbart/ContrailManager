#ifndef SEGMENT
#define SEGMENT

#include <vector>
#include <string>
#include "mapUtils.h"

// Structure for holding points for interpolation plus their weights
struct Interp {
    std::vector<IDX3> points{4};
    std::vector<float> weights{4};
};

// Contrail segment structure
struct Segment {
    std::string parentID = "none"; // ID of the flight object which created the segment
    CMTime birthTime;
    Geo3D back; // Location of back (/rear/start) of segment
    Geo3D front; // Location of front (/end) of segment
    Geo3D centre; // Location of centre of segment; derived from back and front

    float length; // Segment length (m)

    // Some kind of data
};

#endif