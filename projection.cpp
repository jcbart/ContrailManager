#include <iostream>
#include <cmath>
#include <ESMC.h>
#include "projection.h"
#include "mapUtils.h"

void Projection::init(int proj_code, float lat1, float lon1, float knowni, float knownj, float dx,
                      float stdlon, float truelat1, float truelat2) {
    int rc;
    std::string msg;
    if (proj_code != PROJ_LC) {
        std::cerr << "CM Projection error: proj_code = " << proj_code << " not supported."
                  << std::endl;
        std::cerr << "CM Projection error: Only PROJ_LC = " << PROJ_LC << " currently supported."
                  << std::endl;
        exit(EXIT_FAILURE);
    }
    if (std::abs(lat1) > 90) {
        std::cerr << "CM Projection error: lat1 = " << lat1 << " not in range (-90,90]."
                  << std::endl;
        exit(EXIT_FAILURE);
    }
    // Add others
    this->code = proj_code;
    this->lat1 = lat1;
    this->lon1 = lon1;
    this->knowni = knowni;
    this->knownj = knownj;
    this->dx = dx;
    this->stdlon = stdlon;
    this->truelat1 = truelat1;
    this->truelat2 = truelat2;

    if (truelat1 < 0) {
        hemi = -1;
    }
    else {
        hemi = 1;
    }
    rebydx = EARTH_RADIUS_M / dx;
    if (std::abs(this->truelat2) > 90) {
        msg = "CM Projection: truelat2 > 90; assuming a tangent.";
        rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
        this->truelat2 = this->truelat1;
    }
    set_lc();
    isInitd = true;
    msg = "CM Projection initialised. lat1 = " + std::to_string(lat1);
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
}

void Projection::set_lc() {
    cone = lc_cone(truelat1, truelat2);
    float deltalon1 = lon1 - stdlon;
    wrap_WE(deltalon1);

    ctl1r = std::cos(truelat1 * RAD_PER_DEG);

    rsw = rebydx * ctl1r/cone
          * std::pow((std::tan((90.*hemi - lat1)*RAD_PER_DEG/2.) /
                      std::tan((90.*hemi - truelat1)*RAD_PER_DEG/2.)), cone);

    float arg = cone * deltalon1 * RAD_PER_DEG;
    polei = 1. - hemi * rsw * std::sin(arg);
    polej = 1. + rsw * std::cos(arg);
}

float Projection::lc_cone(float truelat1, float truelat2) {
    float cone;
    if (std::abs(truelat1-truelat2) > 0.01) {
        cone = std::log10(std::cos(truelat1*RAD_PER_DEG))
               - std::log10(std::cos(truelat2*RAD_PER_DEG));
        cone /= (std::log10(std::tan((45. - std::abs(truelat1)/2.)*RAD_PER_DEG))
                 - std::log10(std::tan((45. - std::abs(truelat2)/2.)*RAD_PER_DEG)));
    }
    else {
        cone = std::sin(std::abs(truelat1*RAD_PER_DEG));
    }
    return cone;
}

// Returns the grid cell ij which loc lies within
// Assumes i and j start at 1
IDX2 Projection::loc_to_ij(const Geo2D& loc) {
    if (code == PROJ_LC) {
        return loc_to_ij_lc(loc);
    }
    std::cerr << "CM Projection error: code not recognised in Projection::loc_to_ij. Stopping."
              << std::endl;
    exit(EXIT_FAILURE);
    return IDX2{0,0};
}

IDX2 Projection::loc_to_ij_lc(const Geo2D& loc) {
    IDX2 ij;

    float deltalon = loc.lon - stdlon;
    wrap_WE(deltalon);

    float rm = rebydx * ctl1r/cone
               * std::pow((std::tan((90.*hemi - loc.lat)*RAD_PER_DEG/2.) /
                           std::tan((90.*hemi - truelat1)*RAD_PER_DEG/2.)), cone);

    float arg = cone * deltalon * RAD_PER_DEG;
    ij.i = hemi * (polei + hemi * rm * std::sin(arg));
    ij.j = hemi * (polej - rm * std::cos(arg));
    return ij;
}