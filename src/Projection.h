#ifndef PROJECTION_H
#define PROJECTION_H

#include "mapUtils.h"

const int PROJ_LC = 1; // Consistent with WRF

class Projection {
private:
    inline IDX2 loc_to_ij_lc(const Geo2D& loc) const {
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

public:
    // Set externally
    int code; // Projection code
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
    double cone; // Cone factor for LC projections
    double polei; // i-location of pole point
    double polej; // j-location of pole point
    double rsw; // Radius to SW corner
    double rebydx; // Earth radius divided by dx
    double ctl1r; // cos(truelat1*RAD_PER_DEG)
    bool isInitd = false;

    void init(int proj_code, double lat1, double lon1, double knowni, double knownj, double dx,
              double stdlon, double truelat1, double truelat2);

    void set_lc();

    double lc_cone(double truelat1, double truelat2);
    
    // Returns the grid cell ij which loc lies within
    // Assumes i and j start at 0
    inline IDX2 loc_to_ij(const Geo2D& loc) const {
        if (code == PROJ_LC) {
            return loc_to_ij_lc(loc);
        }
        std::cerr << "CM Projection error: code not recognised in Projection::loc_to_ij. Stopping."
                << std::endl;
        exit(EXIT_FAILURE);
        return IDX2{0,0};
    }
};

#endif