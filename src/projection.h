#ifndef PROJECTION_H
#define PROJECTION_H

#include "mapUtils.h"

const int PROJ_LC = 1; // Consistent with WRF

class Projection {
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

    IDX2 loc_to_ij(const Geo2D& loc) const;

    IDX2 loc_to_ij_lc(const Geo2D& loc) const;
};

#endif