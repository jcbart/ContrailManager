#ifndef SEGMENT
#define SEGMENT

#include "mapUtils.h"

// Contrail segment structure
struct Segment {
    Geo3D back;
    Geo3D front;
    Geo3D centre;

    float length;
    float age; // in seconds

    // Some kind of data
};

#endif