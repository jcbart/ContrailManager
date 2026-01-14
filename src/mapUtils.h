#ifndef MAPUTILS_H
#define MAPUTILS_H

#include <string>

// Forward declaration
struct CMTime;
class Domain;

const double EARTH_RADIUS_M = 6.37e6; // Earth radius (m); consistent with WRF

// Forward declarations
struct Geo2D;
struct Geo3D;

// A structure to define a location in geographic (lat, lon) coordinates
struct Geo2D {
    double lon; // degrees, West is negative
    double lat; // degrees, South is negative
    operator Geo3D() const;
    std::string asString() const;
};

// A structure to define a location in geodetic (lat, lon, alt) coordinates
struct Geo3D {
    double lon; // degrees, West is negative
    double lat; // degrees, South is negative
    double alt; // metres above mean sea level
    operator Geo2D() const;
    std::string asString() const;
};

// A structure to define a location (or vector) in Cartesian (x, y, z) coordinates
struct Cart3D {
    double x;
    double y;
    double z;

    // Sum two Cart3D points
    Cart3D operator+(const Cart3D& other) const {
        Cart3D result;
        result.x = this->x + other.x;
        result.y = this->y + other.y;
        result.z = this->z + other.z;
        return result;
    }

    // Subtract one Cart3D point from another
    Cart3D operator-(const Cart3D& other) const {
        Cart3D result;
        result.x = this->x - other.x;
        result.y = this->y - other.y;
        result.z = this->z - other.z;
        return result;
    }
};

// Forward declarations
struct IDX2;
struct IDX3;

// A structure to store 2 integer indices
struct IDX2 {
    int i;
    int j;
    operator IDX3() const;
    std::string asString() const;
};

// A structure to store 3 integer indices
struct IDX3 {
    int i;
    int j;
    int k;
    operator IDX2() const;
    std::string asString() const;
};

Cart3D Geo2D_to_Cart3D(const Geo2D& pointIn);

Geo2D Cart3D_to_Geo2D(const Cart3D& pointIn);

Cart3D Geo3D_to_Cart3D(const Geo3D& pointIn);

Geo3D Cart3D_to_Geo3D(const Cart3D& pointIn);

float vector_mag(const Cart3D& vec);

float cart_dist(const Cart3D& pointA, const Cart3D& pointB);

float cart_dist(const Geo3D& pointA, const Geo3D& pointB);

float great_circle_dist(const Geo3D& pointA, const Geo3D& pointB);

Geo3D great_circle_interp(const double f, const Geo3D& loc1, const Geo3D& loc2);

Geo3D great_circle_interp(const CMTime& time, const CMTime& time1, const Geo3D& loc1,
                          const CMTime& time2, const Geo3D& loc2);

void wrap_WE(double& lon);

void wrap_SN(double& lon, double& lat);

float dot_prod(const Cart3D& vecA, const Cart3D& vecB);

Cart3D cross_prod(const Cart3D& vecA, const Cart3D& vecB);

bool loc_in_quad(const Geo2D& loc, const Geo2D& point1, const Geo2D& point2, const Geo2D& point3, const Geo2D& point4);

bool advect_loc(Geo3D& loc, const float duration_s, const Domain& dom);

bool advect_loc_RK4(Geo3D& loc, const float duration_s, const Domain& dom);

#endif