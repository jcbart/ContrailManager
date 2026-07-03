#include <format>
#include "domain/Domain.h"
#include "map/functions.h"
#include "timekeeping.h"

template <typename ProjType>
Domain::Domain(int ids, int ide, int jds, int jde, int kds, int kde, ProjType p)
    : ids(ids), ide(ide), jds(jds), jde(jde), kds(kds), kde(kde),
      iSize(ide - ids + 1), jSize(jde - jds + 1), kSize(kde - kds + 1),
      // Initialise grids
      grid2D({ids, jds}, {ide, jde}),
      grid3D({ids, jds, kds}, {ide, jde, kde}),
      grid3D_stag_k({ids, jds, kds}, {ide, jde, kde + 1}),
      // Initialise variables
      XLONG("XLONG", grid2D, 0),
      XLAT("XLAT", grid2D, 0),
      Z("Z", grid3D, 0),
      Z_AT_W("Z_AT_W", grid3D_stag_k, 0),
      DRYMASS("DRYMASS", grid3D, 0),
      T_POT("T_POT", grid3D, 0),
      delta_T_POT("delta_T_POT", grid3D, 0),
      T_POT_tend("T_POT_tend", grid3D, 0),
      P("P", grid3D, 0),
      U("U", grid3D, 0),
      V("V", grid3D, 0),
      W("W", grid3D, 0),
      TNSR("TNSR", grid2D, 0),
      OLR("OLR", grid2D, 0),
      QV("QV", grid3D, 0),
      //QVsave("QVsave", grid3D, 0),
      delta_QV("delta_QV", grid3D, 0),
      QV_tend("QV_tend", grid3D, 0),
      QI("QI", grid3D, 0),
      delta_QI("delta_QI", grid3D, 0),
      QI_tend("QI_tend", grid3D, 0),
      delta_NI("delta_NI", grid3D, 0),
      NI_tend("NI_tend", grid3D, 0),
      QIcontrail("QIcontrail", grid3D, 0),
      NIcontrail("NIcontrail", grid3D, 0),
      REIcontrail("REIcontrail", grid3D, 0),
      proj(std::move(p)) {

    CM_LogWrite("Contrail Manager variables initialised with dimensions:");
    CM_LogWrite(std::format("ids = {}, jds = {}, kds = {}", ids, jds, kds));
    CM_LogWrite(std::format("ide = {}, jde = {}, kde = {}", ide, jde, kde));
    CM_LogWrite("Note: Contrail Manager does not use a staggered grid except for Z_AT_W where "
        "kde += 1.");
}

// Methods to compile
template Domain::Domain(int ids, int ide, int jds, int jde, int kds, int kde, LambertConformal p);
template Domain::Domain(int ids, int ide, int jds, int jde, int kds, int kde, LatLon p);

void Domain::construct_tendencies(const CMTime& startTime, const CMTime& stopTime) {
    const double dt_s = (stopTime - startTime).to_s();
    
    for (size_t i = 0; i < QV_tend.grid.get_num_elements(); i++) {
        QV_tend.get_data()[i] = delta_QV.get_data()[i] / dt_s;
    }
    for (size_t i = 0; i < QI_tend.grid.get_num_elements(); i++) {
        QI_tend.get_data()[i] = delta_QI.get_data()[i] / dt_s;
    }
    for (size_t i = 0; i < NI_tend.grid.get_num_elements(); i++) {
        NI_tend.get_data()[i] = delta_NI.get_data()[i] / dt_s;
    }
    for (size_t i = 0; i < T_POT_tend.grid.get_num_elements(); i++) {
        T_POT_tend.get_data()[i] = delta_T_POT.get_data()[i] / dt_s;
    }
}

void Domain::check_valid_exports() const {
    // Define lambda functions
    constexpr auto isNotNegative = [](float x) -> bool { return x >= 0; };
    constexpr auto isFinite = [](float x) -> bool { return std::isfinite(x); }; // includes NaN
    constexpr auto isNotInsane = [](float x) -> bool {return x < 1; };

    delta_T_POT.check_condition(isFinite);
    T_POT_tend.check_condition(isFinite);
    delta_QV.check_condition(isFinite);
    delta_QV.check_condition(isNotInsane);
    QV_tend.check_condition(isFinite);
    delta_QI.check_condition(isFinite);
    delta_QI.check_condition(isNotInsane);
    QI_tend.check_condition(isFinite);
    delta_NI.check_condition(isFinite);
    NI_tend.check_condition(isFinite);
    QIcontrail.check_condition(isFinite);
    QIcontrail.check_condition(isNotNegative);
    NIcontrail.check_condition(isFinite);
    NIcontrail.check_condition(isNotNegative);
    REIcontrail.check_condition(isFinite);
    REIcontrail.check_condition(isNotNegative);
}

bool Domain::find_interp_points(const Geo3D& loc, std::vector<IDX<3, int>>& interpPoints) const {
    // Find grid cell and relevant diagonal
    IDX<2, int> ij, ijDiag;
    if (!loc_to_ij_and_diag(loc, ij, ijDiag)) {
        return false;
    }

    // Construct points
    IDX<2, int> ij1{ij[0], ij[1]};
    IDX<2, int> ij2{ij[0], ijDiag[1]};
    IDX<2, int> ij3{ijDiag[0], ijDiag[1]};
    IDX<2, int> ij4{ijDiag[0], ij[1]};

    interpPoints.resize(8);

    // Find k for each of the four grid points
    // Return false if no k found; else, update interp point
    int k;
    // Point 1
    if (!find_k_below(loc, ij1, k)) {
        return false;
    }
    interpPoints[0].set({ij1[0], ij1[1], k});
    interpPoints[1].set({ij1[0], ij1[1], k+1});

    // Point 2
    if (!find_k_below(loc, ij2, k)) {
        return false;
    }
    interpPoints[2].set({ij2[0], ij2[1], k});
    interpPoints[3].set({ij2[0], ij2[1], k+1});

    // Point 3
    if (!find_k_below(loc, ij3, k)) {
        return false;
    }
    interpPoints[4].set({ij3[0], ij3[1], k});
    interpPoints[5].set({ij3[0], ij3[1], k+1});

    // Point 4
    if (!find_k_below(loc, ij4, k)) {
        return false;
    }
    interpPoints[6].set({ij4[0], ij4[1], k});
    interpPoints[7].set({ij4[0], ij4[1], k+1});
    
    // All points found, return true
    return true;
}

void Domain::find_interp_weights(const Geo3D& loc, const std::vector<IDX<3, int>>& interpPoints,
    std::vector<double>& interpWeights) const {
    
    int numInterpPoints = interpPoints.size();
    interpWeights.resize(numInterpPoints);
    std::vector<double> dists(numInterpPoints);

    // Find distances
    bool anyZero = false;
    for (int i = 0; i < numInterpPoints; i++) {
        dists[i] = map::cart_dist(loc, ijk_to_loc(interpPoints[i]));
        if (dists[i] == 0) { anyZero = true; }
    }
    
    // Find weights
    double totalWeight = 0;
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

bool Domain::wind_at_loc(const Geo3D& loc, double& u, double& v, double& w) const {
    std::vector<IDX<3, int>> interpPoints;
    std::vector<double> interpWeights;
    if (!find_interp_points(loc, interpPoints)) {
        return false;
    }

    find_interp_weights(loc, interpPoints, interpWeights);

    // Values at loc
    u = 0;
    v = 0;
    w = 0;
    // Find values at loc
    for (int i = 0; i < interpPoints.size(); i++) {
        u += U.get(interpPoints[i]) * interpWeights[i];
        v += V.get(interpPoints[i]) * interpWeights[i];
        w += W.get(interpPoints[i]) * interpWeights[i];
    }
    return true;
}