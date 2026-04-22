#ifndef MAPFUNCTIONS_H
#define MAPFUNCTIONS_H

#include <cmath>
#include "constants.h"
#include "map/types.h"

// Forward declarations
class Domain;
struct Waypoint;
struct CMTime;

namespace map {

// Finds the length of a vector (or distance of a point from the origin)
constexpr double vector_mag(const Cart3D& vec) {
    return std::sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
}

// Converts a geographic point to a Cartesian point on a unit circle
inline Cart3D Geo2D_to_Cart3D(const Geo2D& pointIn) {
    Cart3D pointOut;
    double theta = constants::RAD_PER_DEG * (90. - pointIn.lat);
    double phi = constants::RAD_PER_DEG * pointIn.lon;
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
    pointOut.lon = phi/constants::RAD_PER_DEG;
    pointOut.lat = 90. - theta/constants::RAD_PER_DEG;
    return pointOut;
}

// Converts a geodetic point to a Cartesian point
inline Cart3D Geo3D_to_Cart3D(const Geo3D& pointIn) {
    Cart3D pointOut;
    double theta = constants::RAD_PER_DEG * (90. - pointIn.lat);
    double phi = constants::RAD_PER_DEG * pointIn.lon;
    double rho = constants::EARTH_RADIUS_M + pointIn.alt;
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
    pointOut.lon = phi/constants::RAD_PER_DEG;
    pointOut.lat = 90. - theta/constants::RAD_PER_DEG;
    pointOut.alt = rho - constants::EARTH_RADIUS_M;
    return pointOut;
}

// Calculates distance between two Cartesian points
inline double cart_dist(const Cart3D& pointA, const Cart3D& pointB) {
    return vector_mag(pointA - pointB);
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

// Calculates the great circle distance between two points at their average altitude (m)
constexpr double great_circle_dist(const Geo3D& pointA, const Geo3D& pointB) {
    return (constants::EARTH_RADIUS_M + 0.5 * (pointA.alt + pointB.alt)) * std::acos(
        std::cos(constants::RAD_PER_DEG * pointA.lat)
        * std::cos(constants::RAD_PER_DEG * pointB.lat)
        * std::cos(constants::RAD_PER_DEG * (pointA.lon - pointB.lon))
        + std::sin(constants::RAD_PER_DEG * pointA.lat)
        * std::sin(constants::RAD_PER_DEG * pointB.lat)
    );
}

// Wraps longitude to be in range (-180, 180]
inline void wrap_WE(double& lon) {
    if (lon > 180) { lon -= 360; }
    else if (lon <= -180) { lon += 360; }
}

// Wraps longitude and latitude if latitude is outside range [-90, 90]
// (reflects at poles)
inline void wrap_SN(double& lon, double& lat) {
    if (lat > 90) {
        lat = 180 - lat;
        lon += 180;
    }
    else if (lat < -90) {
        lat = -180 - lat;
        lon += 180;
    }
    wrap_WE(lon);
}

// Returns angle between the line from loc 1 (lon1, lat1) to loc 2 (lon2, lat2) and North
// (degrees)
constexpr double great_circle_bearing(const double lon1, const double lat1, const double lon2,
    const double lat2) {

    double lon1_rad = constants::RAD_PER_DEG * lon1;
    double lat1_rad = constants::RAD_PER_DEG * lat1;
    double lon2_rad = constants::RAD_PER_DEG * lon2;
    double lat2_rad = constants::RAD_PER_DEG * lat2;

    double d_lon = lon2_rad - lon1_rad;
    double alpha = constants::DEG_PER_RAD * std::atan2(
        std::sin(d_lon),
        std::cos(lat1_rad) * std::tan(lat2_rad) - std::sin(lat1_rad) * std::cos(d_lon)
    );
    return alpha;
}

// Returns angle between the line from loc 1 to loc 2 and North (degrees)
template <typename GeoType>
constexpr double great_circle_bearing(const GeoType& loc1, const GeoType& loc2) {
    return great_circle_bearing(loc1.lon, loc1.lat, loc2.lon, loc2.lat);
}

// Returns location at a fraction f [0,1] along a great circle by interpolating
// between two waypoints
// If f = 0, the returned location will be loc1 and vice versa for f = 1
Geo3D great_circle_interp(const double f, const Geo3D& loc1, const Geo3D& loc2);

// Returns location at time by interpolating between two waypoints
// If time = wp1.time, the returned location will be wp1.loc and vice versa
Geo3D great_circle_interp(const CMTime& time, const Waypoint& wp1, const Waypoint& wp2);

// Advect a location (loc) for duration in seconds given a domain (dom)
// Updates loc
// Returns false if loc is outside or goes outside the grid
bool advect_loc(Geo3D& loc, const float duration_s, const Domain& dom);

// Option to advect a location (loc) with Runge-Kutta 4th order for duration in seconds given a
// domain (dom)
// Updates loc
// Returns false if loc is outside or goes outside the grid
bool advect_loc_RK4(Geo3D& loc, const float duration_s, const Domain& dom);

}

#endif