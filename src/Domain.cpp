#include <format>
#include "Domain.h"
#include "mapFunctions.h"

template <typename ProjType>
Domain::Domain(int ids, int ide, int jds, int jde, int kds, int kde, ProjType p)
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
      QIcontrail("QIcontrail", ids, ide, jds, jde, kds, kde),
      proj(std::move(p)) {

    CM_LogWrite("Contrail Manager variables initialised with dimensions:");
    CM_LogWrite(std::format("ids = {}, jds = {}, kds = {}", ids, jds, kds));
    CM_LogWrite(std::format("ide = {}, jde = {}, kde = {}", ide, jde, kde));
    CM_LogWrite("Note: Contrail Manager does not use a staggered grid except for Z_AT_W where kde += 1.");
}

// Methods to compile
template Domain::Domain(int ids, int ide, int jds, int jde, int kds, int kde, ProjectionLC p);

bool Domain::find_interp_points(const Geo3D& loc, std::vector<IDX3<int>>& interpPoints) const {
    // Find grid cell and relevant diagonal
    IDX2<int> ij, ijDiag;
    bool inGrid = loc_to_ij_and_diag(loc, ij, ijDiag);

    // Construct points
    IDX2<int> ij1, ij2, ij3, ij4;
    ij1.set(ij.i, ij.j);
    ij2.set(ij.i, ijDiag.j);
    ij3.set(ijDiag.i, ijDiag.j);
    ij4.set(ijDiag.i, ij.j);

    interpPoints.resize(8);

    // Find k for each of the four grid points
    // Return false if no k found; else, update interp point
    int k;
    bool inQuad;
    // Point 1
    inQuad = find_k_below(loc, ij1, k);
    if (!inQuad) { return inQuad; }
    interpPoints[0].set(ij1.i, ij1.j, k);
    interpPoints[1].set(ij1.i, ij1.j, k+1);

    // Point 2
    inQuad = find_k_below(loc, ij2, k);
    if (!inQuad) { return inQuad; }
    interpPoints[2].set(ij2.i, ij2.j, k);
    interpPoints[3].set(ij2.i, ij2.j, k+1);

    // Point 3
    inQuad = find_k_below(loc, ij3, k);
    if (!inQuad) { return inQuad; }
    interpPoints[4].set(ij3.i, ij3.j, k);
    interpPoints[5].set(ij3.i, ij3.j, k+1);

    // Point 4
    inQuad = find_k_below(loc, ij4, k);
    if (!inQuad) { return inQuad; }
    interpPoints[6].set(ij4.i, ij4.j, k);
    interpPoints[7].set(ij4.i, ij4.j, k+1);
    
    // All points found, return true
    return inQuad;
}

void Domain::find_interp_weights(const Geo3D& loc, const std::vector<IDX3<int>>& interpPoints,
    std::vector<float>& interpWeights) const {
    
    int numInterpPoints = interpPoints.size();
    interpWeights.resize(numInterpPoints);
    std::vector<float> dists(numInterpPoints);

    // Find distances
    bool anyZero = false;
    for (int i = 0; i < numInterpPoints; i++) {
        dists[i] = cart_dist(loc, ijk_to_loc(interpPoints[i]));
        if (dists[i] == 0) { anyZero = true; }
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

bool Domain::wind_at_loc(const Geo3D& loc, float& u, float& v, float& w) const {
    bool inGrid;
    std::vector<IDX3<int>> interpPoints;
    std::vector<float> interpWeights;
    inGrid = find_interp_points(loc, interpPoints);
    if (!inGrid) { return false; }

    find_interp_weights(loc, interpPoints, interpWeights);

    // Values at loc
    u = 0;
    v = 0;
    w = 0;
    // Find values at loc
    for (int i = 0; i < interpPoints.size(); i++) {
        u += U.get_value(interpPoints[i]) * interpWeights[i];
        v += V.get_value(interpPoints[i]) * interpWeights[i];
        w += W.get_value(interpPoints[i]) * interpWeights[i];
    }
    return true;
}