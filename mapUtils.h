#ifndef MAPUTILS
#define MAPUTILS

const double PI = 3.14159265358979323846264338327950288419716939937510582;
const double RAD_PER_DEG = PI/180;
const double EARTH_RADIUS_M = 6.37e6; // Earth radius (m); consistent with WRF

// A structure to define a location in geographic (lat, lon) coordinates
struct Geo2D {
    float lat; // degrees, South is negative
    float lon; // degrees, West is negative
};

// A structure to define a location in geodetic (lat, lon, alt) coordinates
struct Geo3D {
    float lat; // degrees, South is negative
    float lon; // degrees, West is negative
    float alt; // metres above mean sea level
};

// A structure to define a location in Cartesian (x, y, z) coordinates
struct Cart3D {
    float x;
    float y;
    float z;
};

// A structure to store 2 integer indices
struct IDX2 {
    int i;
    int j;
};

// A structure to store 3 integer indices
struct IDX3 {
    int i;
    int j;
    int k;
};

Cart3D Geo2D_to_Cart3D(Geo2D pointIn);

Geo2D Cart3D_to_Geo2D(Cart3D pointIn);

Cart3D Geo3D_to_Cart3D(Geo3D pointIn);

Geo3D Cart3D_to_Geo3D(Cart3D pointIn);

#endif