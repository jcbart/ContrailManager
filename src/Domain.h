#ifndef DOMAIN_H
#define DOMAIN_H

#include <memory>
#include <string>
#include <vector>
#include <variant>
#include "mapUtils.h"
#include "Projection.h"
#include "CMLog.h"

template <typename T>
class Variable2D {
private:
    std::string name;
    const int ids, ide, jds, jde;
    const int i_size, j_size;
    const size_t num_elements;

    const T* get_element_ptr(const int i, const int j) const {
        if (i < ids || i > ide || j < jds || j > jde) {
            std::string msg = "Variable2D " + name + " error: Index (i,j)=("
                + std::to_string(i) + "," + std::to_string(j)
                + ") is out of range for array of size (ids:ide,jds:jde)=("
                + std::to_string(ids) + ":" + std::to_string(ide) + ","
                + std::to_string(jds) + ":" + std::to_string(jde) + ")";
            CM_RaiseError(msg, __FILE__, __LINE__);
        }
        return &data[get_1D_index_from_2D(i, j)];
    }

public:
    T* data = nullptr;

    // Destructor
    ~Variable2D() {
        if (data != nullptr) {
            delete[] data;
        }
    }

    // Constructor
    Variable2D(std::string name, int ids, int ide, int jds, int jde)
        : name(name), ids(ids), ide(ide), jds(jds), jde(jde),
          i_size(ide-ids+1), j_size(jde-jds+1),
          num_elements(i_size*j_size) {
        
        // Allocate a 1D block of memory
        data = new T[num_elements];
        clear_all();
    }

    int get_ids() const { return ids; };
    int get_ide() const { return ide; };
    int get_jds() const { return jds; };
    int get_jde() const { return jde; };
    int get_i_size() const { return i_size; };
    int get_j_size() const { return j_size; };
    size_t get_num_elements() const { return num_elements; };

    // Flatten 2D indices
    inline size_t get_1D_index_from_2D(const int i, const int j) const {
        return static_cast<size_t>((i-ids)*j_size + (j-jds));
    }

    // Returns a reference to the value, so can be used to set and get
    inline T* get(const int i, const int j) {
        return const_cast<T*>(get_element_ptr(i, j));
    }

    // Returns a reference to the value, so can be used to set and get
    inline T* get(const IDX2<int>& ij) {
        return get(ij.i, ij.j);
    }

    // Returns the value at indices
    inline T get_value(const int i, const int j) const {
        return *get_element_ptr(i, j);
    }

    // Returns the value at indices
    inline T get_value(const IDX2<int>& ij) const {
        return get_value(ij.i, ij.j);
    }

    // Set all values to zero; only works if initialised
    void clear_all() {
        for (size_t i = 0; i < num_elements; i++) {
            data[i] = 0;
        }
    }
};

template <typename T>
class Variable3D {
private:
    std::string name;
    const int ids, ide, jds, jde, kds, kde;
    const int i_size, j_size, k_size;
    const size_t num_elements;

    const T* get_element_ptr(const int i, const int j, const int k) const {
        if (i < ids || i > ide || j < jds || j > jde || k < kds || k > kde) {
            std::string msg = "Variable3D " + name + " error: Index (i,j,k)=("
                + std::to_string(i) + "," + std::to_string(j) + "," + std::to_string(k)
                + ") is out of range for array of size (ids:ide,jds:jde,kds:kde)=("
                + std::to_string(ids) + ":" + std::to_string(ide) + ","
                + std::to_string(jds) + ":" + std::to_string(jde) + ")"
                + std::to_string(kds) + ":" + std::to_string(kde) + ")";
            CM_RaiseError(msg, __FILE__, __LINE__);
        }
        return &data[get_1D_index_from_3D(i, j, k)];
    }

public:
    T* data = nullptr;
    
    // Destructor
    ~Variable3D() {
        if (data != nullptr) {
            delete[] data;
        }
    }

    // Constructor
    Variable3D(std::string name, int ids, int ide, int jds, int jde, int kds, int kde)
        : name(name), ids(ids), ide(ide), jds(jds), jde(jde), kds(kds), kde(kde),
          i_size(ide-ids+1), j_size(jde-jds+1), k_size(kde-kds+1),
          num_elements(i_size*j_size*k_size) {
        
        // Allocate a 1D block of memory
        data = new T[num_elements];
        clear_all();
    }

    int get_ids() const { return ids; };
    int get_ide() const { return ide; };
    int get_jds() const { return jds; };
    int get_jde() const { return jde; };
    int get_kds() const { return kds; };
    int get_kde() const { return kde; };
    int get_i_size() const { return i_size; };
    int get_j_size() const { return j_size; };
    int get_k_size() const { return k_size; };
    size_t get_num_elements() const { return num_elements; };

    inline size_t get_1D_index_from_3D(const int i, const int j, const int k) const {
        return static_cast<size_t>((i-ids)*j_size*k_size + (j-jds)*k_size + (k-kds));
    }

    // Returns a reference to the value, so can be used to set and get
    inline T* get(const int i, const int j, const int k) {
        return const_cast<T*>(get_element_ptr(i, j, k));
    }

    // Returns a reference to the value, so can be used to set and get
    inline T* get(const IDX3<int>& ijk) {
        return get(ijk.i, ijk.j, ijk.k);
    }

    // Returns the value at indices
    inline T get_value(const int i, const int j, const int k) const {
        return *get_element_ptr(i, j, k);
    }

    // Returns the value at indices
    inline T get_value(const IDX3<int>& ijk) const {
        return get_value(ijk.i, ijk.j, ijk.k);
    }

    // Set all values to zero; only works if initialised
    void clear_all() {
        for (size_t i = 0; i < num_elements; i++) {
            data[i] = 0;
        }
    }
};

class Domain {
private:
    // End indices are one smaller than in WRF because CM only uses a staggered grid for Z_AT_W
    // Longitude is i/x/u direction
    // Latitude is j/y/v direction
    // Altitude is k/z/w direction
    const int ids, ide, jds, jde, kds, kde;
    const int lonSize, latSize, altSize;

    // Projection (std::variant type defined in Projection.h)
    ProjVariant proj;

public:
    bool twoWayCoupling; // True for two-way coupling (feedback to NWP)
    // Meteorological variables (accessible externally)
    Variable2D<float> XLONG; // Longitude (degrees, West is negative)
    Variable2D<float> XLAT; // Latitude (degrees, South is negative)
    Variable3D<float> Z; // Height above sea level at cell centre (m)
    Variable3D<float> Z_AT_W; // Height above sea level at cell interfaces (staggered in z-direction; m)
    Variable3D<float> DRYMASS; // Dry mass in grid cell (kg)
    Variable3D<float> T_POT; // Potential temperature (K)
    Variable3D<float> P; // Total air pressure (Pa)
    Variable3D<float> U; // Wind speed in Eastward direction (m s-1)
    Variable3D<float> V; // Wind speed in Northward direction (m s-1)
    Variable3D<float> W; // Wind speed in vertical direction (m s-1)
    Variable2D<float> TNSR; // Net (downwards) shortwave radiation at TOA (W m-2)
    Variable2D<float> OLR; // Outgoing longwave radiation at TOA (W m-2)
    Variable3D<float> QV; // Water vapour mass mixing ratio (kg (kg dry air-1))
    Variable3D<float> QVsave; // Water vapour mass mixing ratio saved at start of coupling interval (kg (kg dry air-1))
    Variable3D<float> deltaQV; // Change in water vapour mass mixing ratio over coupling interval (kg (kg dry air-1))
    Variable3D<float> QI; // Ice mass mixing ratio excl. live contrails (kg (kg dry air-1))
    Variable3D<float> deltaQI; // Change in ice mass mixing ratio excl. live contrails (kg (kg dry air-1))
    //Variable3D<float> NI; // Ice number mixing ratio excl. live contrails (# (kg dry air-1))
    Variable3D<float> deltaNI; // Change in ice number mixing ratio excl. live contrails (# (kg dry air-1))
    Variable3D<float> QIcontrail; // Contrail ice mass mixing ratio (kg (kg dry air-1))

    int get_ids() const { return ids; }
    int get_ide() const { return ide; }
    int get_jds() const { return jds; }
    int get_jde() const { return jde; }
    int get_kds() const { return kds; }
    int get_kde() const { return kde; }
    int get_lonSize() const { return lonSize; };
    int get_latSize() const { return latSize; };
    int get_altSize() const { return altSize; };

    // Constructor
    template <typename ProjType>
    Domain(int ids, int ide, int jds, int jde, int kds, int kde, ProjType p);

    // Destructor
    ~Domain() = default;

    // Copy contents of QV to QVsave
    void save_QV() {
        for (size_t i = 0; i < QV.get_num_elements(); i++) {
            QVsave.data[i] = QV.data[i];
        }
    }

    // Update deltaQV with deltaQV = QV - QVsave
    void find_deltaQV() {
        for (size_t i = 0; i < deltaQV.get_num_elements(); i++) {
            deltaQV.data[i] = QV.data[i] - QVsave.data[i];
        }
    }

    // Finds the index k such that loc.alt is inside grid cell ijk
    // Updates k in argument
    // Returns false if no valid k found
    inline bool find_k_inside(const Geo3D& loc, const IDX2<int>& ij, int& k) const {
        // Check if below first boundary
        if (loc.alt < Z_AT_W.get_value(ij.i, ij.j, kds)) {
            return false;
        }
        for (int kTrial = kds; kTrial <= kde; kTrial++) {
            if (loc.alt < Z_AT_W.get_value(ij.i, ij.j, kTrial+1)) {
                k = kTrial;
                return true;
            }
        }
        // Else, no valid k found
        return false;
    }

    // Finds the index k such that grid centre altitude at k is less than loc.alt and grid centre
    // altitude at k+1 is greater than loc.alt
    // Updates k in argument
    // Returns false if no valid k found
    inline bool find_k_below(const Geo3D& loc, const IDX2<int>& ij, int& k) const {
        // Check if below first centre
        if (loc.alt < Z.get_value(ij.i, ij.j, kds)) {
            return false;
        }
        for (int kTrial = kds; kTrial < kde; kTrial++) {
            if (loc.alt < Z.get_value(ij.i, ij.j, kTrial+1)) {
                k = kTrial;
                return true;
            }
        }
        // Else, no valid k found
        return false;
    }

    // Updates ij with the lon/lat grid cell indices loc lies within
    // ij may include fractions depending on its type
    // Calls the method in Domain::proj and removes ids = jds = 0 assumption
    // Returns false if loc is not in grid
    template <typename dtype>
    inline bool loc_to_ij(const Geo2D& loc, IDX2<dtype>& ij) const {
        std::visit([&](auto const& p) {
            ij = p.loc_to_ij(loc);
        }, proj);
        // Correct assumption that i and j start at 0
        ij.i += ids;
        ij.j += jds;
        if (ij.i < ids && ij.i > ide && ij.j < jds && ij.j > jde) {
            return false;
        }
        return true;
    }

    // Updates ijk with the lon/lat/alt grid cell indices which loc lies within
    // Returns false if loc is not in grid
    inline bool loc_to_ijk(const Geo3D& loc, IDX3<int>& ijk) const {
        bool inGrid;
        // Get ij
        IDX2<int> ij;
        inGrid = loc_to_ij(loc, ij);
        if (!inGrid) { return false; }
        // Turn IDX2 object into IDX3
        ijk = ij;
        // Get k
        inGrid = find_k_inside(loc, ijk, ijk.k);
        return inGrid;
    }

    // Given a location (loc), find the grid cell ij and its diagonal ijDiag which, alongside
    // their common adjacent grid cells, bound loc (used in interpolation)
    // Updates ij and ijDiag
    // Returns false if either ij or ijDiag are not in the grid
    inline bool loc_to_ij_and_diag(const Geo3D& loc, IDX2<int>& ij, IDX2<int>& ijDiag) const {
        IDX2<double> ijD; // ij as a double
        bool inGrid = loc_to_ij(loc, ijD);
        if (!inGrid) { return false; }

        ij = ijD; // Convert to IDX2<int>

        ijDiag.i = (ijD.i - floor(ijD.i) < 0.5) ? (ij.i - 1) : (ij.i + 1);
        ijDiag.j = (ijD.j - floor(ijD.j) < 0.5) ? (ij.j - 1) : (ij.j + 1);
        if ((ijDiag.i < ids) || (ijDiag.i > ide) || (ijDiag.j < jds) || (ijDiag.j > jde)) {
            // Diag grid cell not in grid
            return false;
        }
        return true;
    }

    // Returns the lon/lat grid values at indices ij
    inline Geo2D ij_to_loc(const IDX2<int>& ij) const {
        Geo2D loc;
        loc.lon = XLONG.get_value(ij.i, ij.j);
        loc.lat = XLAT.get_value(ij.i, ij.j);
        return loc;
    }

    // Returns the lat/lon/alt grid values at indices ijk
    inline Geo3D ijk_to_loc(const IDX3<int>& ijk) const {
        Geo3D loc;
        loc.lon = XLONG.get_value(ijk.i, ijk.j);
        loc.lat = XLAT.get_value(ijk.i, ijk.j);
        loc.alt = Z.get_value(ijk);
        return loc;
    }

    // Returns true if it is possible to do grid interpolation for loc
    // Currently, this means that loc is not outside the outermost layer of grid cell centres
    inline bool can_do_interp(const Geo3D& loc) const {
        IDX2<int> ij, ijDiag;
        bool canDoInterp = loc_to_ij_and_diag(loc, ij, ijDiag);
        return canDoInterp;
    }

    // Finds interpolation points for a location and resizes and updates interpPoints
    // Returns true if location is in grid
    // If false, interp contains garbage
    bool find_interp_points(const Geo3D& loc, std::vector<IDX3<int>>& interpPoints) const;

    // Finds inverse-distance weights for a vector of interpolation points
    void find_interp_weights(const Geo3D& loc, const std::vector<IDX3<int>>& interpPoints,
        std::vector<float>& interpWeights) const;

    // Finds the wind speed at location by interpolating between neighbouring grid cells
    // Updates u, v, and w
    // Returns false if location is not in grid
    bool wind_at_loc(const Geo3D& loc, float& u, float& v, float& w) const;
};

#endif