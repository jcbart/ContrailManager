#include <cmath>
#include <string>
#include "mapUtils.h"
#include "Domain.h"
#include "timekeeping.h"

// Types to compile
template struct IDX2<int>;
template struct IDX2<double>;
template struct IDX3<int>;
template struct IDX3<double>;

// Returns location at a fraction f [0,1] along a great circle by interpolating
// between two waypoints
// If f = 0, the returned location will be loc1 and vice versa for f = 1
Geo3D great_circle_interp(const double f, const Geo3D& loc1, const Geo3D& loc2) {
    // Lat/lon interpolation
    // Pass Geo2D version of Geo3D objects to function
    Cart3D loc1_Cart = Geo2D_to_Cart3D(loc1);
    Cart3D loc2_Cart = Geo2D_to_Cart3D(loc2);
    // Dot product is clamped in range [-1, 1] to prevent precision errors
    double delta = std::acos(std::max(-1., std::min(1., dot_prod(loc1_Cart, loc2_Cart))));
    Cart3D slerpResult;
    // If delta is tiny, resort to LERP
    if (delta < 1e-9) {
        slerpResult.x = (1-f)*loc1_Cart.x + f*loc2_Cart.x;
        slerpResult.y = (1-f)*loc1_Cart.y + f*loc2_Cart.y;
        slerpResult.z = (1-f)*loc1_Cart.z + f*loc2_Cart.z;
    }
    else {
        double slerp1 = std::sin((1-f) * delta) / std::sin(delta);
        double slerp2 = std::sin(f * delta) / std::sin(delta);
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
    double f = (time - time1).to_s()/(time2 - time1).to_s();
    return great_circle_interp(f, loc1, loc2);
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
    double dot1 = dot_prod(cartLoc, n1);
    double dot2 = dot_prod(cartLoc, n2);
    double dot3 = dot_prod(cartLoc, n3);
    double dot4 = dot_prod(cartLoc, n4);
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
    if (!inGrid) { return false; }

    // Advect in longitude
    loc.lon += DEG_PER_RAD * u * duration_s
               / ((EARTH_RADIUS_M + loc.alt) * std::cos(RAD_PER_DEG * loc.lat));
    // Wrap around the Earth
    wrap_WE(loc.lon);

    // Advect in latitude
    loc.lat += DEG_PER_RAD * v * duration_s / (EARTH_RADIUS_M + loc.alt);
    // Reflect at poles
    wrap_SN(loc.lon, loc.lat);

    // Advect in altitude
    loc.alt += w * duration_s;

    // Check if still in grid (able to interp)
    inGrid = dom.can_do_interp(loc);
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
    if (!inGrid) { return false; }

    // Advect in longitude
    loc1.lon = loc.lon + DEG_PER_RAD * u1 * 0.5*duration_s
                         / ((EARTH_RADIUS_M + loc.alt) * std::cos(RAD_PER_DEG * loc.lat));
    // Wrap around the Earth
    wrap_WE(loc1.lon);

    // Advect in latitude
    loc1.lat = loc.lat + DEG_PER_RAD * v1 * 0.5*duration_s / (EARTH_RADIUS_M + loc.alt);
    // Reflect at poles
    wrap_SN(loc1.lon, loc1.lat);

    // Advect in altitude
    loc1.alt = loc.alt + w1 * 0.5*duration_s;

    // k2

    inGrid = dom.wind_at_loc(loc1, u2, v2, w2);
    if (!inGrid) { return false; }

    // Advect in longitude
    loc2.lon = loc.lon + DEG_PER_RAD * u2 * 0.5*duration_s
                         / ((EARTH_RADIUS_M + loc.alt) * std::cos(RAD_PER_DEG * loc.lat));
    // Wrap around the Earth
    wrap_WE(loc2.lon);

    // Advect in latitude
    loc2.lat = loc.lat + DEG_PER_RAD * v2 * 0.5*duration_s / (EARTH_RADIUS_M + loc.alt);
    // Reflect at poles
    wrap_SN(loc2.lon, loc2.lat);

    // Advect in altitude
    loc2.alt = loc.alt + w2 * 0.5*duration_s;

    // k3

    inGrid = dom.wind_at_loc(loc2, u3, v3, w3);
    if (!inGrid) { return false; }

    // Advect in longitude
    loc3.lon = loc.lon + DEG_PER_RAD * u3 * duration_s
                         / ((EARTH_RADIUS_M + loc.alt) * std::cos(RAD_PER_DEG * loc.lat));
    // Wrap around the Earth
    wrap_WE(loc3.lon);

    // Advect in latitude
    loc3.lat = loc.lat + DEG_PER_RAD * v3 * duration_s / (EARTH_RADIUS_M + loc.alt);
    // Reflect at poles
    wrap_SN(loc3.lon, loc3.lat);

    // Advect in altitude
    loc3.alt = loc.alt + w3 * duration_s;

    // k4

    inGrid = dom.wind_at_loc(loc3, u4, v4, w4);
    if (!inGrid) { return false; }

    // Update loc

    // Advect in longitude
    loc.lon += DEG_PER_RAD * (u1 + 2*u2 + 2*u3 + u4) * duration_s/6. 
               / ((EARTH_RADIUS_M + loc.alt) * std::cos(RAD_PER_DEG * loc.lat));
    // Wrap around the Earth
    wrap_WE(loc.lon);

    // Advect in latitude
    loc.lat += DEG_PER_RAD * (v1 + 2*v2 + 2*v3 + v4) * duration_s/6. / (EARTH_RADIUS_M + loc.alt);
    // Reflect at poles
    wrap_SN(loc.lon, loc.lat);

    // Advect in altitude
    loc.alt += (w1 + 2*w2 + 2*w3 + w4) * duration_s/6.;

    // Check if still in grid (able to interp)
    inGrid = dom.can_do_interp(loc);
    return inGrid;
}