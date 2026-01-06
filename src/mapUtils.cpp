#include <cmath>
#include <string>
#include <sstream>
#include "mapUtils.h"
#include "domain.h"
#include "timekeeping.h"

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
    pointOut.x = std::sin(theta) * std::cos(phi);
    pointOut.y = std::sin(theta) * std::sin(phi);
    pointOut.z = std::cos(theta);
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
    float theta = std::acos(pointIn.z);
    float phi = std::atan2(pointIn.y, pointIn.x);
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
    pointOut.x = rho * std::sin(theta) * std::cos(phi);
    pointOut.y = rho * std::sin(theta) * std::sin(phi);
    pointOut.z = rho * std::cos(theta);
    return pointOut;
}

// Converts a Cartesian point to a geodetic point
Geo3D Cart3D_to_Geo3D(const Cart3D& pointIn) {
    Geo3D pointOut;
    float rho = vector_mag(pointIn);
    float theta = std::acos(pointIn.z/rho);
    float phi = std::atan2(pointIn.y, pointIn.x);
    pointOut.lat = 90. - theta/RAD_PER_DEG;
    pointOut.lon = phi/RAD_PER_DEG;
    pointOut.alt = rho - EARTH_RADIUS_M;
    return pointOut;
}

// Finds the length of a vector (or distance of a point from the origin)
float vector_mag(const Cart3D& vec) {
    return std::sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
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
           * std::acos(std::cos(pointA.lat*RAD_PER_DEG)
                       * std::cos(pointB.lat*RAD_PER_DEG)
                       * std::cos((pointA.lon - pointB.lon)*RAD_PER_DEG)
                       + std::sin(pointA.lat*RAD_PER_DEG)
                       * std::sin(pointB.lat*RAD_PER_DEG));
}

// Returns location at a fraction f [0,1] along a great circle by interpolating
// between two waypoints
// If f = 0, the returned location will be loc1 and vice versa for f = 1
Geo3D great_circle_interp(const float f, const Geo3D& loc1, const Geo3D& loc2) {
    // Lat/lon interpolation
    // Pass Geo2D version of Geo3D objects to function
    Cart3D loc1_Cart = Geo2D_to_Cart3D(loc1);
    Cart3D loc2_Cart = Geo2D_to_Cart3D(loc2);
    // Dot product is clamped in range [-1, 1] to prevent precision errors
    float delta = std::acos(std::max(-1.F, std::min(1.F, dot_prod(loc1_Cart, loc2_Cart))));
    Cart3D slerpResult;
    // If delta is tiny, resort to LERP
    if (delta < 1e-9) {
        slerpResult.x = (1-f)*loc1_Cart.x + f*loc2_Cart.x;
        slerpResult.y = (1-f)*loc1_Cart.y + f*loc2_Cart.y;
        slerpResult.z = (1-f)*loc1_Cart.z + f*loc2_Cart.z;
    }
    else {
        float slerp1 = std::sin((1-f) * delta) / std::sin(delta);
        float slerp2 = std::sin(f * delta) / std::sin(delta);
        slerpResult.x = slerp1*loc1_Cart.x + slerp2*loc2_Cart.x;
        slerpResult.y = slerp1*loc1_Cart.y + slerp2*loc2_Cart.y;
        slerpResult.z = slerp1*loc1_Cart.z + slerp2*loc2_Cart.z;
    }
    // Pass Geo2D into Geo3D object
    Geo3D result = Cart3D_to_Geo2D(slerpResult);

    // Altitude interpolation
    result.alt = (1-f) * loc1.alt + f * loc2.alt;
    return result;
}

// Returns location at time by interpolating between two waypoints given times
// If time = time1, the returned location will be loc1 and vice versa
Geo3D great_circle_interp(const CMTime& time, const CMTime& time1, const Geo3D& loc1,
                          const CMTime& time2, const Geo3D& loc2) {
    float time_s = time.dhms_to_s();
    float time1_s = time1.dhms_to_s();
    float time2_s = time2.dhms_to_s();
    float f = (time_s - time1_s)/(time2_s - time1_s);
    return great_circle_interp(f, loc1, loc2);
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

// Advect a location (loc) for duration in seconds given a domain (dom)
// Updates loc
// Returns false if loc is outside or goes outside the grid
bool advect_loc(Geo3D& loc, const float duration_s, const Domain& dom) {
    bool inGrid;

    float u, v, w;

    inGrid = dom.wind_at_loc(loc, u, v, w);
    if (!inGrid) {return inGrid;}

    // Advect in longitude
    loc.lon += u * duration_s / ((EARTH_RADIUS_M + loc.alt) * std::cos(loc.lat)) * DEG_PER_RAD;
    // Wrap around the Earth
    wrap_WE(loc.lon);

    // Advect in latitude
    loc.lat += v * duration_s / (EARTH_RADIUS_M + loc.alt) * DEG_PER_RAD;
    // Reflect at poles
    wrap_SN(loc.lon, loc.lat);

    // Advect in altitude
    loc.alt += w * duration_s;

    // Check if still in grid
    std::vector<IDX3> interpTemp;
    inGrid = dom.find_interp_points(loc, interpTemp);
    return inGrid;
}

// Option to advect a location (loc) with Runge-Kutta 4th order for duration in seconds given a
// domain (dom)
// Updates loc
// Returns false if loc is outside or goes outside the grid
bool advect_loc_RK4(Geo3D& loc, const float duration_s, const Domain& dom) {
    bool inGrid;

    float u1, v1, w1; // Values of k1
    float u2, v2, w2; // Values of k2
    float u3, v3, w3; // Values of k3
    float u4, v4, w4; // Values of k4

    Geo3D loc1; // Location after k1 step, used in k2 step
    Geo3D loc2; // Location after k2 step, used in k3 step
    Geo3D loc3; // Location after k3 step, used in k4 step

    // k1

    inGrid = dom.wind_at_loc(loc, u1, v1, w1);
    if (!inGrid) {return inGrid;}

    // Advect in longitude
    loc.lon = loc.lon + u1 * 0.5*duration_s / ((EARTH_RADIUS_M + loc.alt) * std::cos(loc.lat))
                          * DEG_PER_RAD;
    // Wrap around the Earth
    wrap_WE(loc1.lon);

    // Advect in latitude
    loc1.lat = loc.lat + v1 * 0.5*duration_s / (EARTH_RADIUS_M + loc.alt) * DEG_PER_RAD;
    // Reflect at poles
    wrap_SN(loc1.lon, loc1.lat);

    // Advect in altitude
    loc1.alt = loc.alt + w1 * 0.5*duration_s;

    // k2

    inGrid = dom.wind_at_loc(loc1, u2, v2, w2);
    if (!inGrid) {return inGrid;}

    // Advect in longitude
    loc2.lon = loc.lon + u2 * 0.5*duration_s / ((EARTH_RADIUS_M + loc.alt) * std::cos(loc.lat))
                          * DEG_PER_RAD;
    // Wrap around the Earth
    wrap_WE(loc2.lon);

    // Advect in latitude
    loc2.lat = loc.lat + v2 * 0.5*duration_s / (EARTH_RADIUS_M + loc.alt) * DEG_PER_RAD;
    // Reflect at poles
    wrap_SN(loc2.lon, loc2.lat);

    // Advect in altitude
    loc2.alt = loc.alt + w2 * 0.5*duration_s;

    // k3

    inGrid = dom.wind_at_loc(loc2, u3, v3, w3);
    if (!inGrid) {return inGrid;}

    // Advect in longitude
    loc3.lon = loc.lon + u3 * duration_s / ((EARTH_RADIUS_M + loc.alt) * std::cos(loc.lat))
                          * DEG_PER_RAD;
    // Wrap around the Earth
    wrap_WE(loc3.lon);

    // Advect in latitude
    loc3.lat = loc.lat + v3 * duration_s / (EARTH_RADIUS_M + loc.alt) * DEG_PER_RAD;
    // Reflect at poles
    wrap_SN(loc3.lon, loc3.lat);

    // Advect in altitude
    loc3.alt = loc.alt + w3 * duration_s;

    // k4

    inGrid = dom.wind_at_loc(loc3, u4, v4, w4);
    if (!inGrid) {return inGrid;}

    // Update loc

    // Advect in longitude
    loc.lon += (u1 + 2*u2 + 2*u3 + u4) * duration_s/6. 
               / ((EARTH_RADIUS_M + loc.alt) * std::cos(loc.lat)) * DEG_PER_RAD;
    // Wrap around the Earth
    wrap_WE(loc.lon);

    // Advect in latitude
    loc.lat += (v1 + 2*v2 + 2*v3 + v4) * duration_s/6. / (EARTH_RADIUS_M + loc.alt) * DEG_PER_RAD;
    // Reflect at poles
    wrap_SN(loc.lon, loc.lat);

    // Advect in altitude
    loc.alt += (w1 + 2*w2 + 2*w3 + w4) * duration_s/6.;

    // Check if still in grid
    std::vector<IDX3> interpTemp;
    inGrid = dom.find_interp_points(loc, interpTemp);
    return inGrid;
}