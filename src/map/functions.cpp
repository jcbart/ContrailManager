#include "map/functions.h"
#include "flight/Waypoint.h"
#include "domain/Domain.h"

Geo3D map::great_circle_interp(const double f, const Geo3D& loc1, const Geo3D& loc2) {
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
    result.alt = (1 - f) * loc1.alt + f * loc2.alt;
    return result;
}

Geo3D map::great_circle_interp(const CMTime& time, const Waypoint& wp1,
    const Waypoint& wp2) {
    
    double f = (time - wp1.time) / (wp2.time - wp1.time);
    return great_circle_interp(f, wp1.loc, wp2.loc);
}

bool map::advect_loc(Geo3D& loc, const float duration_s, const Domain& dom) {
    double u, v, w;
    if (!dom.wind_at_loc(loc, u, v, w)) { return false; }

    // Advect in longitude
    loc.lon += constants::DEG_PER_RAD * u * duration_s
               / ((constants::EARTH_RADIUS_M + loc.alt) * std::cos(constants::RAD_PER_DEG * loc.lat));
    // Wrap around the Earth
    wrap_WE(loc.lon);

    // Advect in latitude
    loc.lat += constants::DEG_PER_RAD * v * duration_s / (constants::EARTH_RADIUS_M + loc.alt);
    // Reflect at poles
    wrap_SN(loc.lon, loc.lat);

    // Advect in altitude
    loc.alt += w * duration_s;

    // Check if still in grid (able to interp)
    return dom.can_do_interp(loc);
}

bool map::advect_loc_RK4(Geo3D& loc, const float duration_s, const Domain& dom) {

    // Lambda function to advance a position by (u, v, w) over dt seconds, using reference geometry
    // from ref_loc
    auto advect_step = [&](const Geo3D& base_loc, const Geo3D& ref_loc,
        const double u, const double v, const double w, const float dt_s) -> Geo3D {
        
        Geo3D next;

        next.lon = base_loc.lon
            + constants::DEG_PER_RAD * u * dt_s
              / ((constants::EARTH_RADIUS_M + ref_loc.alt)
                 * std::cos(constants::RAD_PER_DEG * ref_loc.lat));
        // Wrap around the Earth
        wrap_WE(next.lon);

        next.lat = base_loc.lat
            + constants::DEG_PER_RAD * v * dt_s / (constants::EARTH_RADIUS_M + ref_loc.alt);
        // Reflect at poles
        wrap_SN(next.lon, next.lat);

        next.alt = base_loc.alt + w * dt_s;

        return next;
    };

    const float half_duration_s = 0.5 * duration_s;

    // Values of k1
    double u1, v1, w1;
    if (!dom.wind_at_loc(loc, u1, v1, w1)) { return false; }
    // Location after k1 step, used in k2 step
    Geo3D loc1 = advect_step(loc, loc, u1, v1, w1, half_duration_s);

    // Values of k2
    double u2, v2, w2;
    if (!dom.wind_at_loc(loc1, u2, v2, w2)) { return false; }
    // Location after k2 step, used in k3 step
    Geo3D loc2 = advect_step(loc, loc1, u2, v2, w2, half_duration_s);

    // Values of k3
    double u3, v3, w3;
    if (!dom.wind_at_loc(loc2, u3, v3, w3)) { return false; }
    // Location after k3 step, used in k4 step
    Geo3D loc3 = advect_step(loc, loc2, u3, v3, w3, duration_s);

    // Values of k4
    double u4, v4, w4;
    if (!dom.wind_at_loc(loc3, u4, v4, w4)) { return false; }

    const double u_rk4 = (u1 + 2*u2 + 2*u3 + u4) / 6.;
    const double v_rk4 = (v1 + 2*v2 + 2*v3 + v4) / 6.;
    const double w_rk4 = (w1 + 2*w2 + 2*w3 + w4) / 6.;
    loc = advect_step(loc, loc, u_rk4, v_rk4, w_rk4, duration_s);

    // Check if still in grid (able to interp)
    return dom.can_do_interp(loc);
}