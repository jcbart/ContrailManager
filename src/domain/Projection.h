#ifndef PROJECTION_H
#define PROJECTION_H

#include <cmath>
#include <variant>
#include "constants.h"
#include "map/functions.h"
#include "CMLog.h"

class Projection {
protected:
    // Set externally
    double lon1; // Longitude of origin cell centre
    double lat1; // Latitude of origin cell centre
    double dx; // Grid spacing (m); unused in LatLon
    double stdlon; // Standard longitude (or lon increment for LatLon)
    double truelat1; // 1st true latitude (or lat increment for LatLon)
    double truelat2; // 2nd true latitude for LambertConformal

    // Set internally
    int hemi; // 1 for NH, -1 for SH
    double rebydx; // Earth radius divided by dx

public:
    virtual ~Projection() = default;

    Projection(double lat1, double lon1, double dx, double stdlon, double truelat1,
        double truelat2)
        : lat1(lat1), lon1(lon1), dx(dx), stdlon(stdlon),
          truelat1(truelat1), truelat2(truelat2) {

        if (std::abs(lat1) > 90) {
            CM_RaiseError(
                std::format("Projection error: lat1 = {} not in range [-90,90].", lat1),
                __FILE__, __LINE__
            );
        }
        if (std::abs(lon1) > 180) {
            CM_RaiseError(
                std::format("Projection error: lon1 = {} not in range [-180,180].", lon1),
                __FILE__, __LINE__
            );
        }
        if (std::abs(truelat1) > 90) {
            CM_RaiseError(
                std::format("Projection error: truelat1 = {} not in range [-90,90].", truelat1),
                __FILE__, __LINE__
            );
        }

        hemi = (truelat1 < 0) ? -1 : 1;

        rebydx = constants::EARTH_RADIUS_M / dx;
    }

    // Returns the grid cell ij which loc lies within
    // Cell ij covers the ranges [i-0.5, i+0.5) and [j-0.5, j+0.5)
    // i.e. the integer grid cell can be found by rounding
    // Assumes i and j are 0 at the origin (lon1, lat1)
    virtual IDX<2, double> loc_to_ij(const Geo2D& loc) const = 0;
};

// Lambert-Conformal projection
class LambertConformal : public Projection {
    double cone; // Cone factor
    double ctl1r; // cos(truelat1*RAD_PER_DEG)
    double thmtl1r; // tan((90*hemi - truelat1) * RAD_PER_DEG/2)
    double rsw; // Radius to SW corner
    double polei; // i-location of pole point
    double polej; // j-location of pole point

    static double lc_cone(double truelat1, double truelat2) {
        double cone;
        if (std::abs(truelat1 - truelat2) > 0.01) {
            cone = std::log10(std::cos(truelat1 * constants::RAD_PER_DEG))
                   - std::log10(std::cos(truelat2 * constants::RAD_PER_DEG));
            cone /= (std::log10(std::tan((45. - std::abs(truelat1)/2.) * constants::RAD_PER_DEG))
                    - std::log10(std::tan((45. - std::abs(truelat2)/2.) * constants::RAD_PER_DEG)));
        }
        else {
            cone = std::sin(std::abs(truelat1 * constants::RAD_PER_DEG));
        }
        return cone;
    }

public:
    LambertConformal(double lat1, double lon1, double dx, double stdlon, double truelat1,
        double truelat2)
        : Projection(lat1, lon1, dx, stdlon, truelat1, truelat2) {

        if (this->dx < 0) {
            CM_RaiseError(
                std::format("LambertConformal error: dx = {} must be positive.", dx),
                __FILE__, __LINE__
            );
        }
        if (std::abs(this->stdlon) > 180) {
            CM_RaiseError(
                std::format("LambertConformal error: stdlon = {} not in range [-180,180].", stdlon),
                __FILE__, __LINE__
            );
        }
        if (std::abs(this->truelat2) > 90) {
            CM_LogWarning("LambertConformal: truelat2 > 90; assuming a tangent.");
            this->truelat2 = this->truelat1;
        }
        
        cone = lc_cone(truelat1, truelat2);
        double deltalon1 = lon1 - stdlon;
        map::wrap_WE(deltalon1);

        ctl1r = std::cos(truelat1 * constants::RAD_PER_DEG);

        thmtl1r = std::tan((90.*hemi - truelat1) * constants::RAD_PER_DEG/2.);

        rsw = rebydx * ctl1r/cone
            * std::pow(std::tan((90.*hemi - lat1) * constants::RAD_PER_DEG/2.) / thmtl1r, cone);

        double arg = cone * deltalon1 * constants::RAD_PER_DEG;
        polei = -hemi * rsw * std::sin(arg);
        polej = rsw * std::cos(arg);
    }

    IDX<2, double> loc_to_ij(const Geo2D& loc) const override {
        IDX<2, double> ij;

        double deltalon = loc.lon - stdlon;
        map::wrap_WE(deltalon);

        double rm = rebydx * ctl1r/cone
            * std::pow(std::tan((90.*hemi - loc.lat) * constants::RAD_PER_DEG/2.) / thmtl1r, cone);

        double arg = cone * deltalon * constants::RAD_PER_DEG;
        ij[0] = hemi * (polei + hemi * rm * std::sin(arg));
        ij[1] = hemi * (polej - rm * std::cos(arg));
        return ij;
    }
};

// Latitude-Longitude (Cylindrical Equidistant) projection
class LatLon : public Projection {
public:
    LatLon(double lat1, double lon1, double dx, double stdlon, double truelat1, double truelat2)
        : Projection(lat1, lon1, dx, stdlon, truelat1, truelat2) {
        
        if (std::abs(this->stdlon) > 180) {
            CM_RaiseError(
                std::format("LatLon error: stdlon = {} not in range [-180,180].", stdlon),
                __FILE__, __LINE__
            );
        }

        if (this->lon1 < 0) {
            this->lon1 += 360;
        }
    }

    IDX<2, double> loc_to_ij(const Geo2D& loc) const override {
        IDX<2, double> ij;

        double deltalat = loc.lat - lat1;

        double deltalon = loc.lon - lon1;

        // Wrap deltalon to [0, 360) for eastward grids (stdlon > 0) or (-360, 0] for
        // westward grids (stdlon < 0)
        if (stdlon > 0) {
            while (deltalon >= 360) { deltalon -= 360; }
            while (deltalon < 0) { deltalon += 360; }
        }
        else {
            while (deltalon > 0) { deltalon -= 360; }
            while (deltalon <= -360) { deltalon += 360; }
        }

        // Where lon/lat increments are stdlon/truelat1
        ij[0] = deltalon / stdlon;
        ij[1] = deltalat / truelat1;
        return ij;
    }
};

// Variant of all Projection options
using ProjVariant = std::variant<
    LambertConformal,
    LatLon
>;

#endif