#include <cmath>
#include <string>
#include <sstream>
#include "mapUtils.h"

// Return a Geo3D version of a Geo2D object (alt not set)
Geo2D::operator Geo3D() const {
    Geo3D as3D;
    as3D.lon = this->lon;
    as3D.lat = this->lat;
    return as3D;
}

// Return location (lon, lat) as string
std::string Geo2D::asString() {
    std::stringstream ss;
    ss << "(" << lon << ", " << lat << ")";
    return ss.str();
}

// Return a Geo2D version of a Geo3D object (alt stripped)
Geo3D::operator Geo2D() const {
    Geo2D as2D;
    as2D.lon = this->lon;
    as2D.lat = this->lat;
    return as2D;
}

// Return location (lon, lat, alt) as string
std::string Geo3D::asString() {
    std::stringstream ss;
    ss << "(" << lon << ", " << lat << ", " << alt << ")";
    return ss.str();
}

// Return a IDX3 version of an IDX2 object (k not set)
IDX2::operator IDX3() const {
    IDX3 as3D;
    as3D.i = this->i;
    as3D.j = this->j;
    return as3D;
}

// Return a IDX2 version of an IDX3 object (k stripped)
IDX3::operator IDX2() const {
    IDX2 as2D;
    as2D.i = this->i;
    as2D.j = this->j;
    return as2D;
}

// Converts a geographic point to a Cartesian point on a unit circle
Cart3D Geo2D_to_Cart3D(const Geo2D& pointIn) {
    Cart3D pointOut;
    float theta = RAD_PER_DEG * (90. - pointIn.lat);
    float phi = RAD_PER_DEG * pointIn.lon;
    pointOut.x = sin(theta) * cos(phi);
    pointOut.y = sin(theta) * sin(phi);
    pointOut.z = cos(theta);
    // Rescale to reduce precision errors
    float rho = vector_mag(pointOut);
    pointOut.x /= rho;
    pointOut.y /= rho;
    pointOut.z /= rho;
    return pointOut;
}

// Converts a Cartesian point on a unit circle to a geographic point
Geo2D Cart3D_to_Geo2D(const Cart3D& pointIn) {
    Geo2D pointOut;
    float theta = acos(pointIn.z);
    float phi = atan2(pointIn.y, pointIn.x);
    pointOut.lat = 90. - theta/RAD_PER_DEG;
    pointOut.lon = phi/RAD_PER_DEG;
    return pointOut;
}

// Converts a geodetic point to a Cartesian point
Cart3D Geo3D_to_Cart3D(const Geo3D& pointIn) {
    Cart3D pointOut;
    float theta = RAD_PER_DEG * (90. - pointIn.lat);
    float phi = RAD_PER_DEG * pointIn.lon;
    float rho = EARTH_RADIUS_M + pointIn.alt;
    pointOut.x = rho * sin(theta) * cos(phi);
    pointOut.y = rho * sin(theta) * sin(phi);
    pointOut.z = rho * cos(theta);
    return pointOut;
}

// Converts a Cartesian point to a geodetic point
Geo3D Cart3D_to_Geo3D(const Cart3D& pointIn) {
    Geo3D pointOut;
    float rho = sqrt(std::pow(pointIn.x, 2) + std::pow(pointIn.y, 2)
                     + std::pow(pointIn.z, 2));
    float theta = acos(pointIn.z/rho);
    float phi = atan2(pointIn.y, pointIn.x);
    pointOut.lat = 90. - theta/RAD_PER_DEG;
    pointOut.lon = phi/RAD_PER_DEG;
    pointOut.alt = rho - EARTH_RADIUS_M;
    return pointOut;
}

// Finds the length of a vector (or distance of a point from the origin)
float vector_mag(const Cart3D& vec) {
    return sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
}

// Calculates distance between two Cartesian points
float cart_dist(const Cart3D& pointA, const Cart3D& pointB) {
    Cart3D diff = pointA - pointB;
    return vector_mag(diff);
}

// Calculates Cartesian (straight line) distance between points
float cart_dist(const Geo3D& pointA, const Geo3D& pointB) {
    return cart_dist(Geo3D_to_Cart3D(pointA), Geo3D_to_Cart3D(pointB));
}

// Calculates the great circle distance between two points at their average altitude
float great_circle_dist(const Geo3D& pointA, const Geo3D& pointB) {
    float alt_avg = 0.5 * (pointA.alt + pointB.alt);
    return (EARTH_RADIUS_M + alt_avg)
           * acos(cos(pointA.lat*RAD_PER_DEG)
                  * cos(pointB.lat*RAD_PER_DEG)
                  * cos((pointA.lon - pointB.lon)*RAD_PER_DEG)
                  + sin(pointA.lat*RAD_PER_DEG)
                  * sin(pointB.lat*RAD_PER_DEG));
}

// Wraps longitude to be in range (-180, 180]
void wrap_WE(float& lon) {
    while (lon > 180) {lon -= 360;}
    while (lon <= -180) {lon += 360;}
}

// Wraps longitude and latitude if latitude is outside range [-90, 90]
// (reflects at poles)
void wrap_SN(float& lon, float& lat) {
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

// Calculates the dot product between two vectors
float dot_prod(const Cart3D& vecA, const Cart3D& vecB) {
    return (vecA.x*vecB.x + vecA.y*vecB.y + vecA.z*vecB.z);
}

Cart3D cross_prod(const Cart3D& vecA, const Cart3D& vecB) {
    Cart3D result;
    result.x = vecA.y*vecB.z - vecA.z*vecB.y;
    result.y = vecA.z*vecB.x - vecA.x*vecB.z;
    result.z = vecA.x*vecB.y - vecA.y*vecB.x;
    return result;
}

// Determines whether a location (loc) is within four points on the surface of a unit circle
// The points must be given in clockwise or anticlockwise order
bool loc_in_quad(const Geo2D& loc, const Geo2D& point1, const Geo2D& point2, const Geo2D& point3, const Geo2D& point4) {
    // Convert to Cartesian
    Cart3D cartLoc = Geo2D_to_Cart3D(loc);
    Cart3D cart1 = Geo2D_to_Cart3D(point1);
    Cart3D cart2 = Geo2D_to_Cart3D(point2);
    Cart3D cart3 = Geo2D_to_Cart3D(point3);
    Cart3D cart4 = Geo2D_to_Cart3D(point4);
    // Find normal vectors from each face (a face is the plane between two points and the origin)
    Cart3D n1 = cross_prod(cart2, cart1);
    Cart3D n2 = cross_prod(cart3, cart2);
    Cart3D n3 = cross_prod(cart4, cart3);
    Cart3D n4 = cross_prod(cart1, cart4);
    // Find the dot product of each face with the location
    float dot1 = dot_prod(cartLoc, n1);
    float dot2 = dot_prod(cartLoc, n2);
    float dot3 = dot_prod(cartLoc, n3);
    float dot4 = dot_prod(cartLoc, n4);
    // Is inside if all dot products agree on sign
    bool isInside = false;
    if (dot1 >= 0 && dot2 >= 0 && dot3 >= 0 && dot4 >= 0) {
        isInside = true;
    }
    else if (dot1 <= 0 && dot2 <= 0 && dot3 <= 0 && dot4 <= 0) {
        isInside = true;
    }
    return isInside;
}