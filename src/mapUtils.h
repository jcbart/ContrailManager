#ifndef MAPUTILS_H
#define MAPUTILS_H

#include <cmath>
#include <string>
#include <sstream>

// Forward declaration
struct CMTime;
class Domain;

constexpr double PI = 3.14159265358979323846264338327950288419716939937510582;
constexpr double RAD_PER_DEG = PI/180;
constexpr double DEG_PER_RAD = 1/RAD_PER_DEG;
constexpr double EARTH_RADIUS_M = 6.37e6; // Earth radius (m); consistent with WRF

// Forward declarations
struct Geo2D;
struct Geo3D;

// A structure to define a location in geographic (lat, lon) coordinates
struct Geo2D {
    double lon; // degrees, West is negative
    double lat; // degrees, South is negative

    // Return a Geo3D version of a Geo2D object (alt not set)
    inline operator Geo3D() const;

    // Return location (lon, lat) as string
    inline std::string asString() const {
        std::stringstream ss;
        ss << "(" << lon << ", " << lat << ")";
        return ss.str();
    }
};

// A structure to define a location in geodetic (lat, lon, alt) coordinates
struct Geo3D {
    double lon; // degrees, West is negative
    double lat; // degrees, South is negative
    double alt; // metres above mean sea level

    // Return a Geo2D version of a Geo3D object (alt stripped)
    inline operator Geo2D() const;

    // Return location (lon, lat, alt) as string
    inline std::string asString() const {
        std::stringstream ss;
        ss << "(" << lon << ", " << lat << ", " << alt << ")";
        return ss.str();
    }
};

// Return a Geo3D version of a Geo2D object (alt not set)
inline Geo2D::operator Geo3D() const {
    Geo3D as3D;
    as3D.lon = this->lon;
    as3D.lat = this->lat;
    return as3D;
}

// Return a Geo2D version of a Geo3D object (alt stripped)
inline Geo3D::operator Geo2D() const {
    Geo2D as2D;
    as2D.lon = this->lon;
    as2D.lat = this->lat;
    return as2D;
}

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

    // Return a IDX3 version of an IDX2 object (k not set)
    inline operator IDX3() const;

    // Return location (i, j) as string
    inline std::string asString() const {
        std::stringstream ss;
        ss << "(" << i << ", " << j << ")";
        return ss.str();
    }
};

// A structure to store 3 integer indices
struct IDX3 {
    int i;
    int j;
    int k;

    // Return a IDX2 version of an IDX3 object (k stripped)
    inline operator IDX2() const;

    // Return location (i, j, k) as string
    inline std::string asString() const {
        std::stringstream ss;
        ss << "(" << i << ", " << j << ", " << k << ")";
        return ss.str();
    }
};

// Return a IDX3 version of an IDX2 object (k not set)
inline IDX2::operator IDX3() const {
    IDX3 as3D;
    as3D.i = this->i;
    as3D.j = this->j;
    return as3D;
}

// Return a IDX2 version of an IDX3 object (k stripped)
inline IDX3::operator IDX2() const {
    IDX2 as2D;
    as2D.i = this->i;
    as2D.j = this->j;
    return as2D;
}

// Finds the length of a vector (or distance of a point from the origin)
inline double vector_mag(const Cart3D& vec) {
    return std::sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
}

// Converts a geographic point to a Cartesian point on a unit circle
inline Cart3D Geo2D_to_Cart3D(const Geo2D& pointIn) {
    Cart3D pointOut;
    double theta = RAD_PER_DEG * (90. - pointIn.lat);
    double phi = RAD_PER_DEG * pointIn.lon;
    pointOut.x = std::sin(theta) * std::cos(phi);
    pointOut.y = std::sin(theta) * std::sin(phi);
    pointOut.z = std::cos(theta);
    // Rescale to reduce precision errors
    double rho = vector_mag(pointOut);
    pointOut.x /= rho;
    pointOut.y /= rho;
    pointOut.z /= rho;
    return pointOut;
}

// Converts a Cartesian point on a unit circle to a geographic point
inline Geo2D Cart3D_to_Geo2D(const Cart3D& pointIn) {
    Geo2D pointOut;
    double theta = std::acos(pointIn.z);
    double phi = std::atan2(pointIn.y, pointIn.x);
    pointOut.lat = 90. - theta/RAD_PER_DEG;
    pointOut.lon = phi/RAD_PER_DEG;
    return pointOut;
}

// Converts a geodetic point to a Cartesian point
inline Cart3D Geo3D_to_Cart3D(const Geo3D& pointIn) {
    Cart3D pointOut;
    double theta = RAD_PER_DEG * (90. - pointIn.lat);
    double phi = RAD_PER_DEG * pointIn.lon;
    double rho = EARTH_RADIUS_M + pointIn.alt;
    pointOut.x = rho * std::sin(theta) * std::cos(phi);
    pointOut.y = rho * std::sin(theta) * std::sin(phi);
    pointOut.z = rho * std::cos(theta);
    return pointOut;
}

// Converts a Cartesian point to a geodetic point
inline Geo3D Cart3D_to_Geo3D(const Cart3D& pointIn) {
    Geo3D pointOut;
    double rho = vector_mag(pointIn);
    double theta = std::acos(pointIn.z/rho);
    double phi = std::atan2(pointIn.y, pointIn.x);
    pointOut.lat = 90. - theta/RAD_PER_DEG;
    pointOut.lon = phi/RAD_PER_DEG;
    pointOut.alt = rho - EARTH_RADIUS_M;
    return pointOut;
}

// Calculates distance between two Cartesian points
inline double cart_dist(const Cart3D& pointA, const Cart3D& pointB) {
    Cart3D diff = pointA - pointB;
    return vector_mag(diff);
}

// Calculates Cartesian (straight line) distance between points
inline double cart_dist(const Geo3D& pointA, const Geo3D& pointB) {
    return cart_dist(Geo3D_to_Cart3D(pointA), Geo3D_to_Cart3D(pointB));
}

// Calculates the dot product between two vectors
constexpr double dot_prod(const Cart3D& vecA, const Cart3D& vecB) {
    return (vecA.x*vecB.x + vecA.y*vecB.y + vecA.z*vecB.z);
}

// Calculates the cross product of two vectors
inline Cart3D cross_prod(const Cart3D& vecA, const Cart3D& vecB) {
    Cart3D result;
    result.x = vecA.y*vecB.z - vecA.z*vecB.y;
    result.y = vecA.z*vecB.x - vecA.x*vecB.z;
    result.z = vecA.x*vecB.y - vecA.y*vecB.x;
    return result;
}

// Calculates the great circle distance between two points at their average altitude
constexpr double great_circle_dist(const Geo3D& pointA, const Geo3D& pointB) {
    return (EARTH_RADIUS_M + 0.5 * (pointA.alt + pointB.alt)) * std::acos(
        std::cos(RAD_PER_DEG * pointA.lat)
        * std::cos(RAD_PER_DEG * pointB.lat)
        * std::cos(RAD_PER_DEG * (pointA.lon - pointB.lon))
        + std::sin(RAD_PER_DEG * pointA.lat)
        * std::sin(RAD_PER_DEG * pointB.lat)
    );
}

// Wraps longitude to be in range (-180, 180]
inline void wrap_WE(double& lon) {
    while (lon > 180) { lon -= 360; }
    while (lon <= -180) { lon += 360; }
}

// Wraps longitude and latitude if latitude is outside range [-90, 90]
// (reflects at poles)
inline void wrap_SN(double& lon, double& lat) {
    while (lat > 90) {
        lat = 180-lat;
        lon += 180;
    }
    while (lat < -90) {
        lat = -180-lat;
        lon += 180;
    }
    wrap_WE(lon);
}

// Returns angle between the line from loc 1 (lon1, lat1) to loc 2 (lon2, lat2) and North
// (degrees)
inline double great_circle_bearing(const double lon1, const double lat1, const double lon2,
    const double lat2) {

    double lon1_rad = RAD_PER_DEG * lon1;
    double lat1_rad = RAD_PER_DEG * lat1;
    double lon2_rad = RAD_PER_DEG * lon2;
    double lat2_rad = RAD_PER_DEG * lat2;

    double d_lon = lon2_rad - lon1_rad;
    double alpha = DEG_PER_RAD * std::atan2(
        std::sin(d_lon),
        std::cos(lat1_rad) * std::tan(lat2_rad) - std::sin(lat1_rad) * std::cos(d_lon)
    );
    return alpha;
}

// Returns angle between the line from loc 1 to loc 2 and North (degrees)
inline double great_circle_bearing(const Geo2D& loc1, const Geo2D& loc2) {
    return great_circle_bearing(loc1.lon, loc1.lat, loc2.lon, loc2.lat);
}

// Returns angle between the line from loc 1 to loc 2 and North (degrees)
inline double great_circle_bearing(const Geo3D& loc1, const Geo3D& loc2) {
    return great_circle_bearing(loc1.lon, loc1.lat, loc2.lon, loc2.lat);
}

Geo3D great_circle_interp(const double f, const Geo3D& loc1, const Geo3D& loc2);

Geo3D great_circle_interp(const CMTime& time, const CMTime& time1, const Geo3D& loc1,
                          const CMTime& time2, const Geo3D& loc2);

bool loc_in_quad(const Geo2D& loc, const Geo2D& point1, const Geo2D& point2, const Geo2D& point3, const Geo2D& point4);

bool advect_loc(Geo3D& loc, const float duration_s, const Domain& dom);

bool advect_loc_RK4(Geo3D& loc, const float duration_s, const Domain& dom);

#endif