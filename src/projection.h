#ifndef PROJECTION_H
#define PROJECTION_H

#include "mapUtils.h"

const int PROJ_LC = 1; // Consistent with WRF

class Projection {
public:
    // Set externally
    int code; // Projection code
    float lat1;
    float lon1;
    float knowni; // X-location of known lat/lon
    float knownj; // Y-location of known lat/lon
    float dx;
    float stdlon;
    float truelat1;
    float truelat2;
    // Set internally
    float dlat; // Lat increment for lat/lon grids
    float dlon; // Lon increment for lat/lon grids
    int hemi; // 1 for NH, -1 for SH
    float cone; // Cone factor for LC projections
    float polei; // i-location of pole point
    float polej; // j-location of pole point
    float rsw; // Radius to SW corner
    float rebydx; // Earth radius divided by dx
    float ctl1r; // cos(truelat1*RAD_PER_DEG)
    bool isInitd = false;

    void init(int proj_code, float lat1, float lon1, float knowni, float knownj, float dx,
              float stdlon, float truelat1, float truelat2);

    void set_lc();

    float lc_cone(float truelat1, float truelat2);

    IDX2 loc_to_ij(const Geo2D& loc) const;

    IDX2 loc_to_ij_lc(const Geo2D& loc) const;
};

#endif