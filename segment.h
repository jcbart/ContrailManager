#ifndef SEGMENT
#define SEGMENT

#include <vector>
#include "mapUtils.h"

// Structure for holding points for interpolation plus their weights
struct Interp {
    std::vector<IDX3> points;
    std::vector<float> weights;

    Interp() {
        points.resize(4);
        weights.resize(4);
    }
};

// Contrail segment structure
struct Segment {
    int parentID = -1; // ID of the flight object which created the segment
    CMTime birthTime;
    Geo3D back; // Location of back (/rear/start) of segment
    Geo3D front; // Location of front (/end) of segment
    Geo3D centre; // Location of centre of segment; derived from back and front

    float length;

    // Some kind of data
};

#endif