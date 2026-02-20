#include <iostream>
#include <string>
#include <vector>
#include <ESMC.h>
#include "Domain.h"
#include "mapUtils.h"

// List of types to compile
template class Variable2D<float>;
template class Variable3D<float>;


IDomain::IDomain(int ids, int ide, int jds, int jde, int kds, int kde)
    : ids(ids), ide(ide), jds(jds), jde(jde), kds(kds), kde(kde),
      lonSize(ide-ids+1), latSize(jde-jds+1), altSize(kde-kds+1),
      XLONG("XLONG", ids, ide, jds, jde),
      XLAT("XLAT", ids, ide, jds, jde),
      Z("Z", ids, ide, jds, jde, kds, kde),
      Z_AT_W("Z_AT_W", ids, ide, jds, jde, kds, kde+1),
      DRYMASS("DRYMASS", ids, ide, jds, jde, kds, kde),
      T_POT("T_POT", ids, ide, jds, jde, kds, kde),
      P("P", ids, ide, jds, jde, kds, kde),
      U("U", ids, ide, jds, jde, kds, kde),
      V("V", ids, ide, jds, jde, kds, kde),
      W("W", ids, ide, jds, jde, kds, kde),
      TNSR("TNSR", ids, ide, jds, jde),
      OLR("OLR", ids, ide, jds, jde),
      QV("QV", ids, ide, jds, jde, kds, kde),
      QVsave("QV", ids, ide, jds, jde, kds, kde),
      deltaQV("deltaQV", ids, ide, jds, jde, kds, kde),
      QI("QI", ids, ide, jds, jde, kds, kde),
      deltaQI("deltaQI", ids, ide, jds, jde, kds, kde),
      deltaNI("deltaNI", ids, ide, jds, jde, kds, kde),
      QIcontrail("QIcontrail", ids, ide, jds, jde, kds, kde) {

    int rc;
    std::string msg;
    msg = "Contrail Manager variables initialised with dimensions:";
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
    msg = "ids = " + std::to_string(ids) + ", jds = " + std::to_string(jds) + ", kds = " + std::to_string(kds);
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
    msg = "ide = " + std::to_string(ide) + ", jde = " + std::to_string(jde) + ", kde = " + std::to_string(kde);
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
    msg = "Note: Contrail Manager does not use a staggered grid except for Z_AT_W where kde += 1.";
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
}

// Finds interpolation points for a location and updates interpPoints
// Returns true if location is in grid
// If false, interp contains garbage
bool IDomain::find_interp_points(const Geo3D& loc, std::vector<IDX3<int>>& interpPoints) const {
    interpPoints.resize(4);
    bool inGrid = false;
    IDX2<int> ijCentre;
    inGrid = loc_to_ij(loc, ijCentre);

    // If inGrid is still false, loc is not inside a grid cell
    if (!inGrid) {return inGrid;}
    
    // Determine existence of neighbouring quadrilaterals
    bool doLeft = true, doRight = true, doLower = true, doUpper = true;
    if (ijCentre.i == ids) {doLeft = false;}
    if (ijCentre.i == ide) {doRight = false;}
    if (ijCentre.j == jds) {doLower = false;}
    if (ijCentre.j == jde) {doUpper = false;}
    IDX2<int> ij1, ij2, ij3, ij4;
    // Set to true if loc is inside a quad (also to avoid excess computation)
    bool inQuad = false;
    if (!inQuad && doLeft && doLower) {
        ij1 = ijCentre;
        ij2 = {ijCentre.i-1, ijCentre.j};
        ij3 = {ijCentre.i-1, ijCentre.j-1};
        ij4 = {ijCentre.i, ijCentre.j-1};
        inQuad = loc_in_quad(loc, ij_to_loc(ij1), ij_to_loc(ij2), ij_to_loc(ij3), ij_to_loc(ij4));
    }
    if (!inQuad && doLeft && doUpper) {
        ij1 = ijCentre;
        ij2 = {ijCentre.i, ijCentre.j+1};
        ij3 = {ijCentre.i-1, ijCentre.j+1};
        ij4 = {ijCentre.i-1, ijCentre.j};
        inQuad = loc_in_quad(loc, ij_to_loc(ij1), ij_to_loc(ij2), ij_to_loc(ij3), ij_to_loc(ij4));
    }
    if (!inQuad && doRight && doUpper) {
        ij1 = ijCentre;
        ij2 = {ijCentre.i+1, ijCentre.j};
        ij3 = {ijCentre.i+1, ijCentre.j+1};
        ij4 = {ijCentre.i, ijCentre.j+1};
        inQuad = loc_in_quad(loc, ij_to_loc(ij1), ij_to_loc(ij2), ij_to_loc(ij3), ij_to_loc(ij4));
    }
    if (!inQuad && doRight && doLower) {
        ij1 = ijCentre;
        ij2 = {ijCentre.i, ijCentre.j-1};
        ij3 = {ijCentre.i+1, ijCentre.j-1};
        ij4 = {ijCentre.i+1, ijCentre.j};
        inQuad = loc_in_quad(loc, ij_to_loc(ij1), ij_to_loc(ij2), ij_to_loc(ij3), ij_to_loc(ij4));
    }
    // If inQuad is still false, no quad has been found with loc inside
    if (!inQuad) {return inQuad;}
    // Find k for each of the four grid points
    // Return false if no k found; else, update interp point
    int k;
    // Point 1
    inQuad = find_k_below(loc, ij1, k);
    if (!inQuad) {return inQuad;}
    else {interpPoints[0] = {ij1.i, ij1.j, k};}
    // Point 2
    inQuad = find_k_below(loc, ij2, k);
    if (!inQuad) {return inQuad;}
    else {interpPoints[1] = {ij2.i, ij2.j, k};}
    // Point 3
    inQuad = find_k_below(loc, ij3, k);
    if (!inQuad) {return inQuad;}
    else {interpPoints[2] = {ij3.i, ij3.j, k};}
    // Point 4
    inQuad = find_k_below(loc, ij4, k);
    if (!inQuad) {return inQuad;}
    else {interpPoints[3] = {ij4.i, ij4.j, k};}
    // All points found, return true
    return inQuad;
}

// Finds inverse-distance weights for a vector of interpolation points
void IDomain::find_interp_weights(const Geo3D& loc, const std::vector<IDX3<int>>& interpPoints,
                                 std::vector<float>& interpWeights) const {
    int numInterpPoints = interpPoints.size();
    interpWeights.resize(numInterpPoints);
    std::vector<float> dists(numInterpPoints);

    // Find distances
    bool anyZero = false;
    for (int i = 0; i < numInterpPoints; i++) {
        dists[i] = cart_dist(loc, ijk_to_loc(interpPoints[i]));
        if (dists[i] == 0) {anyZero = true;}
    }
    
    // Find weights
    float totalWeight = 0;
    if (anyZero) {
        for (int i = 0; i < numInterpPoints; i++) {
            interpWeights[i] = (dists[i] == 0) ? 1 : 0;
            totalWeight += interpWeights[i];
        }
    }
    else {
        for (int i = 0; i < numInterpPoints; i++) {
            interpWeights[i] = 1/dists[i];
            totalWeight += interpWeights[i];
        }
    }
    // Scale weights
    for (int i = 0; i < numInterpPoints; i++) {
        interpWeights[i] /= totalWeight;
    }
}

// Finds the wind speed at location by interpolating between neighbouring grid cells
// Updates u, v, and w
// Returns false if location is not in grid
bool IDomain::wind_at_loc(const Geo3D& loc, float& u, float& v, float& w) const {
    bool inGrid;
    std::vector<IDX3<int>> interpPoints;
    std::vector<float> interpWeights;
    inGrid = find_interp_points(loc, interpPoints);
    if (!inGrid) {return inGrid;}

    find_interp_weights(loc, interpPoints, interpWeights);

    int numInterpPoints = interpPoints.size();
    // Values at loc
    u = 0;
    v = 0;
    w = 0;
    // Find values at loc
    for (int i = 0; i < numInterpPoints; i++) {
        u += U.get_value(interpPoints[i]) * interpWeights[i];
        v += V.get_value(interpPoints[i]) * interpWeights[i];
        w += W.get_value(interpPoints[i]) * interpWeights[i];
    }
    return inGrid;
}