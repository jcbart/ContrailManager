#ifndef DOMAIN_H
#define DOMAIN_H

#include <memory>
#include <vector>
#include <algorithm>
#include <ranges>
#include "Variable.h"
#include "map/types.h"
#include "Projection.h"
#include "thermo.h"
#include "CMLog.h"

class Domain {
private:
    // CM does not use a staggered grid except for Z_AT_W
    const int ids; // Grid starting index in dimension i
    const int ide; // Grid ending index in dimension i
    const int jds; // Grid starting index in dimension j
    const int jde; // Grid ending index in dimension j
    const int kds; // Grid starting index in dimension k
    const int kde; // Grid ending index in dimension k
    const size_t iSize, jSize, kSize;

    // Projection (variant type defined in Projection.h)
    ProjVariant proj;

public:
    bool twoWayCoupling; // True for two-way coupling (feedback to NWP)
    // Meteorological variables (accessible externally)
    // Longitude (degrees, West is negative)
    Variable<2, float> XLONG;
    // Latitude (degrees, South is negative)
    Variable<2, float> XLAT;
    // Height above sea level at cell centre (m)
    Variable<3, float> Z;
    // Height above sea level at cell interfaces (staggered in z-direction; m)
    Variable<3, float> Z_AT_W;
    // Dry mass in grid cell (kg)
    Variable<3, float> DRYMASS;
    // Potential temperature (K)
    Variable<3, float> T_POT;
    // Change in potential temperature (K)
    //Variable<3, float> deltaT_POT;
    // Total air pressure (Pa)
    Variable<3, float> P;
    // Wind speed in Eastward direction (m s-1)
    Variable<3, float> U;
    // Wind speed in Northward direction (m s-1)
    Variable<3, float> V;
    // Wind speed in vertical direction (m s-1)
    Variable<3, float> W;
    // Net (downwards) shortwave radiation at TOA (W m-2)
    Variable<2, float> TNSR;
    // Outgoing longwave radiation at TOA (W m-2)
    Variable<2, float> OLR;
    // Water vapour mass mixing ratio (kg (kg dry air-1))
    Variable<3, float> QV;
    // Water vapour mass mixing ratio saved at start of coupling interval (kg (kg dry air-1))
    //Variable<3, float> QVsave;
    // Change in water vapour mass mixing ratio over coupling interval (kg (kg dry air-1))
    Variable<3, float> deltaQV;
    // Ice mass mixing ratio excl. live contrails (kg (kg dry air-1))
    Variable<3, float> QI;
    // Change in ice mass mixing ratio excl. live contrails (kg (kg dry air-1))
    Variable<3, float> deltaQI;
    // Ice number mixing ratio excl. live contrails (# (kg dry air-1))
    //Variable<3, float> NI;
    // Change in ice number mixing ratio excl. live contrails (# (kg dry air-1))
    Variable<3, float> deltaNI;
    // Contrail ice mass mixing ratio (kg (kg dry air-1))
    Variable<3, float> QIcontrail;
    // Contrail ice effective radius (m) (zero if no contrail in cell)
    Variable<3, float> REIcontrail;

    int get_ids() const { return ids; }
    int get_ide() const { return ide; }
    int get_jds() const { return jds; }
    int get_jde() const { return jde; }
    int get_kds() const { return kds; }
    int get_kde() const { return kde; }
    size_t get_iSize() const { return iSize; };
    size_t get_jSize() const { return jSize; };
    size_t get_kSize() const { return kSize; };

    // Constructor
    template <typename ProjType>
    Domain(int ids, int ide, int jds, int jde, int kds, int kde, ProjType p);

    // Destructor
    ~Domain() = default;

    /*
    // Copy contents of QV to QVsave
    void save_QV() {
        for (size_t i = 0; i < QV.get_num_elements(); i++) {
            QVsave.get_data()[i] = QV.get_data()[i];
        }
    }
    */

    /*
    // Update deltaQV with deltaQV = QV - QVsave
    void find_deltaQV() {
        for (size_t i = 0; i < deltaQV.get_num_elements(); i++) {
            deltaQV.get_data()[i] = QV.get_data()[i] - QVsave.get_data()[i];
        }
    }
    */

    /*
    // Calculate deltaT_POT due to deltaQI (sublimation/deposition)
    void find_deltaT_POT() {
        for (size_t i = 0; i < deltaT_POT.get_num_elements(); i++) {
            double slhs = thermo::slh_sublimation_ice(
                thermo::theta_to_T(T_POT.get_data()[i], P.get_data()[i])
            );
            double delta_T = slhs * deltaQI.get_data()[i] / constants::c_pd;
            deltaT_POT.get_data()[i] = thermo::T_to_theta(delta_T, P.get_data()[i]);
        }
    }
    */

    // Checks the export fields have valid values; raises an error if not
    void check_valid_exports() const;
    
    // Finds the index k such that loc.alt is inside grid cell ijk
    // Updates k in argument
    // Returns false if no valid k found
    inline bool find_k_inside(const Geo3D& loc, const IDX<2, int>& ij, int& k) const {
        // Check if below first or above last boundary
        if (loc.alt < Z_AT_W.get({ij[0], ij[1], kds}) ||
            loc.alt >= Z_AT_W.get({ij[0], ij[1], kde + 1})) {
            return false;
        }
        // Binary search within range
        int left = kds, right = kde + 1;
        while (left < right) {
            int mid = left + (right - left) / 2;
            if (loc.alt < Z_AT_W.get({ij[0], ij[1], mid + 1})) {
                right = mid;
            }
            else {
                left = mid + 1;
            }
        }
        // If valid k found, set and return
        if (left <= kde) {
            k = left;
            return true;
        }
        // Else, no valid k found
        return false;
    }

    // Finds the index k such that grid centre altitude at k is less than loc.alt and grid centre
    // altitude at k+1 is greater than loc.alt
    // Updates k in argument
    // Returns false if no valid k found
    inline bool find_k_below(const Geo3D& loc, const IDX<2, int>& ij, int& k) const {
        // Check if below first or above last centre
        if (loc.alt < Z.get({ij[0], ij[1], kds}) ||
            loc.alt >= Z.get({ij[0], ij[1], kde})) {
            return false;
        }
        // Binary search within range
        int left = kds, right = kde;
        while (left < right) {
            int mid = left + (right - left) / 2;
            if (loc.alt < Z.get({ij[0], ij[1], mid + 1})) {
                right = mid;
            }
            else {
                left = mid + 1;
            }
        }
        // If valid k found, set and return
        if (left < kde) {
            k = left;
            return true;
        }
        // Else, no valid k found
        return false;
    }

    // Updates ij with the lon/lat grid cell indices loc lies within
    // ij may include fractions depending on its type
    // Calls the method in Domain::proj and removes ids = jds = 0 assumption
    // Returns false if loc is not in grid
    template <typename T>
    inline bool loc_to_ij(const Geo2D& loc, IDX<2, T>& ij) const {
        std::visit([&](auto const& p) {
            ij = p.loc_to_ij(loc);
        }, proj);
        // Correct assumption that i and j start at 0
        ij[0] += ids;
        ij[1] += jds;
        if (ij[0] < ids || ij[0] > ide || ij[1] < jds || ij[1] > jde) {
            return false;
        }
        return true;
    }

    // Returns the lon/lat grid cell indices which loc lies within
    // Return may include fractions depending on its type
    // Calls the method in Domain::proj and removes ids = jds = 0 assumption
    // Exits if loc is not in grid
    // If exiting is not desired, use alternative loc_to_ij method
    template <typename T>
    inline IDX<2, T> loc_to_ij(const Geo2D& loc) const {
        IDX<2, T> ij;
        if (!loc_to_ij(loc, ij)) [[unlikely]] {
            CM_RaiseUnexpectedOutOfBounds(loc, __FILE__, __LINE__);
        }
        return ij;
    }

    // Updates ijk with the lon/lat/alt grid cell indices which loc lies within
    // Returns false if loc is not in grid
    inline bool loc_to_ijk(const Geo3D& loc, IDX<3, int>& ijk) const {
        // Get ij
        IDX<2, int> ij;
        if (!loc_to_ij(loc, ij)) {
            return false;
        }
        // Turn IDX2 object into IDX3
        ijk = ij;
        // Get k
        return find_k_inside(loc, ijk, ijk[2]);
    }

    // Returns the lon/lat/alt grid cell indices which loc lies within
    // Exits if loc is not in grid
    // If exiting is not desired, use alternative loc_to_ijk method
    inline IDX<3, int> loc_to_ijk(const Geo3D& loc) const {
        IDX<3, int> ijk;
        if (!loc_to_ijk(loc, ijk)) [[unlikely]] {
            CM_RaiseUnexpectedOutOfBounds(loc, __FILE__, __LINE__);
        }
        return ijk;
    }

    // Given a location (loc), find the grid cell ij and its diagonal ijDiag which, alongside
    // their common adjacent grid cells, bound loc (used in interpolation)
    // Updates ij and ijDiag
    // Returns false if either ij or ijDiag are not in the grid
    inline bool loc_to_ij_and_diag(const Geo3D& loc, IDX<2, int>& ij, IDX<2, int>& ijDiag) const {
        IDX<2, double> ijD; // ij as a double
        if (!loc_to_ij(loc, ijD)) {
            return false;
        }

        ij = ijD; // Convert to IDX<2, int>

        ijDiag[0] = (ijD[0] - floor(ijD[0]) < 0.5) ? (ij[0] - 1) : (ij[0] + 1);
        ijDiag[1] = (ijD[1] - floor(ijD[1]) < 0.5) ? (ij[1] - 1) : (ij[1] + 1);
        if (ijDiag[0] < ids || ijDiag[0] > ide || ijDiag[1] < jds || ijDiag[1] > jde) {
            // Diag grid cell not in grid
            return false;
        }
        return true;
    }

    // Returns the lon/lat grid values at indices ij
    inline Geo2D ij_to_loc(const IDX<2, int>& ij) const {
        Geo2D loc;
        loc.lon = XLONG.get(ij);
        loc.lat = XLAT.get(ij);
        return loc;
    }

    // Returns the lat/lon/alt grid values at indices ijk
    inline Geo3D ijk_to_loc(const IDX<3, int>& ijk) const {
        Geo3D loc;
        loc.lon = XLONG.get(ijk);
        loc.lat = XLAT.get(ijk);
        loc.alt = Z.get(ijk);
        return loc;
    }

    // Returns true if it is possible to do grid interpolation for loc
    // Currently, this means that loc is not outside the outermost layer of grid cell centres
    inline bool can_do_interp(const Geo3D& loc) const {
        std::vector<IDX<3, int>> interPoints;
        return (find_interp_points(loc, interPoints));
    }

    // Finds interpolation points for a location and resizes and updates interpPoints
    // Returns true if location is in grid
    // If false, interp contains garbage
    bool find_interp_points(const Geo3D& loc, std::vector<IDX<3, int>>& interpPoints) const;

    // Finds inverse-distance weights for a vector of interpolation points
    void find_interp_weights(const Geo3D& loc, const std::vector<IDX<3, int>>& interpPoints,
        std::vector<double>& interpWeights) const;

    // Finds the wind speed at location by interpolating between neighbouring grid cells
    // Updates u, v, and w
    // Returns false if location is not in grid
    bool wind_at_loc(const Geo3D& loc, float& u, float& v, float& w) const;
};

#endif