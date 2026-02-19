#ifndef PROJECTION_H
#define PROJECTION_H

#include <iostream>
#include <cmath>
#include <ESMC.h>
#include "mapUtils.h"

const int PROJ_LC = 1; // Consistent with WRF

class IProjection {
protected:
    // Set externally
    double lat1;
    double lon1;
    double knowni; // X-location of known lat/lon
    double knownj; // Y-location of known lat/lon
    double dx;
    double stdlon;
    double truelat1;
    double truelat2;

    // Set internally
    double dlat; // Lat increment for lat/lon grids
    double dlon; // Lon increment for lat/lon grids
    int hemi; // 1 for NH, -1 for SH
    double polei; // i-location of pole point
    double polej; // j-location of pole point
    double rsw; // Radius to SW corner
    double rebydx; // Earth radius divided by dx

public:
    virtual ~IProjection() = default;

    IProjection(double lat1, double lon1, double knowni, double knownj, double dx,
        double stdlon, double truelat1, double truelat2)
        : lat1(lat1), lon1(lon1), knowni(knowni), knownj(knownj), dx(dx), stdlon(stdlon),
          truelat1(truelat1), truelat2(truelat2) {

        if (std::abs(lat1) > 90) {
            std::cerr << "CM Projection error: lat1 = " << lat1 << " not in range (-90,90]."
                    << std::endl;
            exit(EXIT_FAILURE);
        }

        hemi = (truelat1 < 0) ? -1 : 1;

        rebydx = EARTH_RADIUS_M / dx;
        if (std::abs(this->truelat2) > 90) {
            std::string msg = "CM Projection: truelat2 > 90; assuming a tangent.";
            int rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
            this->truelat2 = this->truelat1;
        }
    }
    
    // Returns the grid cell ij which loc lies within
    // Assumes i and j start at 0
    virtual inline IDX2 loc_to_ij(const Geo2D& loc) const = 0;
};

// Lambert-Conformal projection
class ProjectionLC : public IProjection {
    double cone; // Cone factor
    double ctl1r; // cos(truelat1*RAD_PER_DEG)

    inline double lc_cone(double truelat1, double truelat2) {
        double cone;
        if (std::abs(truelat1 - truelat2) > 0.01) {
            cone = std::log10(std::cos(truelat1 * RAD_PER_DEG))
                   - std::log10(std::cos(truelat2 * RAD_PER_DEG));
            cone /= (std::log10(std::tan((45. - std::abs(truelat1)/2.) * RAD_PER_DEG))
                    - std::log10(std::tan((45. - std::abs(truelat2)/2.) * RAD_PER_DEG)));
        }
        else {
            cone = std::sin(std::abs(truelat1 * RAD_PER_DEG));
        }
        return cone;
    }

public:
    ProjectionLC(double lat1, double lon1, double knowni, double knownj, double dx,
        double stdlon, double truelat1, double truelat2)
        : IProjection(lat1, lon1, knowni, knownj, dx, stdlon, truelat1, truelat2) {
        
        cone = lc_cone(truelat1, truelat2);
        double deltalon1 = lon1 - stdlon;
        wrap_WE(deltalon1);

        ctl1r = std::cos(truelat1 * RAD_PER_DEG);

        rsw = rebydx * ctl1r/cone
            * std::pow((std::tan((90.*hemi - lat1) * RAD_PER_DEG/2.) /
                        std::tan((90.*hemi - truelat1) * RAD_PER_DEG/2.)), cone);

        double arg = cone * deltalon1 * RAD_PER_DEG;
        polei = 1. - hemi * rsw * std::sin(arg);
        polej = 1. + rsw * std::cos(arg);
    }

    inline IDX2 loc_to_ij(const Geo2D& loc) const override {
        IDX2 ij;

        double deltalon = loc.lon - stdlon;
        wrap_WE(deltalon);

        double rm = rebydx * ctl1r/cone
                * std::pow((std::tan((90.*hemi - loc.lat) * RAD_PER_DEG/2.) /
                            std::tan((90.*hemi - truelat1) * RAD_PER_DEG/2.)), cone);

        double arg = cone * deltalon * RAD_PER_DEG;
        ij.i = hemi * (polei + hemi * rm * std::sin(arg));
        ij.j = hemi * (polej - rm * std::cos(arg));
        return ij;
    }
};

#endif