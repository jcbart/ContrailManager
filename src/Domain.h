#ifndef DOMAIN_H
#define DOMAIN_H

#include <memory>
#include <string>
#include <vector>
#include "mapUtils.h"
#include "Projection.h"

template <typename T>
class Variable2D {
private:
    std::string name = "UNDEFINED";
    int ids = 0, ide = 0, jds = 0, jde = 0;
    int i_size = 0, j_size = 0;
    size_t num_elements = 0;
    bool isInitialised = false;

    const T* get_element_ptr(const int i, const int j) const;

public:
    T* data = nullptr;

    // Destructor
    ~Variable2D();

    void init(std::string name, int ids, int ide, int jds, int jde);

    int get_ids() const {return ids;};
    int get_ide() const {return ide;};
    int get_jds() const {return jds;};
    int get_jde() const {return jde;};
    int get_i_size() const {return i_size;};
    int get_j_size() const {return j_size;};
    size_t get_num_elements() const {return num_elements;};

    size_t get_1D_index_from_2D(const int i, const int j) const;

    T* get(const int i, const int j);
    T* get(const IDX2<int>& ij);

    T get_value(const int i, const int j) const;
    T get_value(const IDX2<int>& ij) const;

    void clear_all();
};

template <typename T>
class Variable3D {
private:
    std::string name = "UNDEFINED";
    int ids = 0, ide = 0, jds = 0, jde = 0, kds = 0, kde = 0;
    int i_size = 0, j_size = 0, k_size = 0;
    size_t num_elements = 0;
    bool isInitialised = false;

    const T* get_element_ptr(const int i, const int j, const int k) const;

public:
    T* data = nullptr;
    
    // Destructor
    ~Variable3D();

    void init(std::string name, int ids, int ide, int jds, int jde, int kds, int kde);

    int get_ids() const {return ids;};
    int get_ide() const {return ide;};
    int get_jds() const {return jds;};
    int get_jde() const {return jde;};
    int get_kds() const {return kds;};
    int get_kde() const {return kde;};
    int get_i_size() const {return i_size;};
    int get_j_size() const {return j_size;};
    int get_k_size() const {return k_size;};
    size_t get_num_elements() const {return num_elements;};

    size_t get_1D_index_from_3D(const int i, const int j, const int k) const;

    T* get(const int i, const int j, const int k);
    T* get(const IDX3<int>& ijk);

    T get_value(const int i, const int j, const int k) const;
    T get_value(const IDX3<int>& ijk) const;

    void clear_all();
};

class Domain {
private:
    // End indices are one smaller than in WRF because CM only uses a staggered grid for Z_AT_W
    // Longitude is i/x/u direction
    // Latitude is j/y/v direction
    // Altitude is k/z/w direction
    int ids = 0, ide = 0, jds = 0, jde = 0, kds = 0, kde = 0;
    int lonSize = 0, latSize = 0, altSize = 0;
    bool varsInitd = false;

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

    // Pointer to projection
    std::unique_ptr<IProjection> proj;

    int get_ids() const {return ids;}
    int get_ide() const {return ide;}
    int get_jds() const {return jds;}
    int get_jde() const {return jde;}
    int get_kds() const {return kds;}
    int get_kde() const {return kde;}
    int get_lonSize() const {return lonSize;};
    int get_latSize() const {return latSize;};
    int get_altSize() const {return altSize;};
    bool get_varsInitd() const {return varsInitd;}

    void init_vars(int ids, int ide, int jds, int jde, int kds, int kde);

    void save_QV();

    void find_deltaQV();

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
    // ij can include fractions or not depending on its dtype
    // Calls the method in Domain::proj and removes ids = jds = 0 assumption
    // Returns false if loc is not in grid
    template <typename dtype>
    inline bool loc_to_ij(const Geo2D& loc, IDX2<dtype>& ij) const {
        bool inGrid = false;
        ij = proj->loc_to_ij(loc);
        // Correct assumption that i and j start at 1
        ij.i += ids;
        ij.j += jds;
        if (ij.i >= ids && ij.i <= ide && ij.j >= jds && ij.j <= jde) {
            inGrid = true;
        }
        return inGrid;
    }

    // Updates ijk with the lon/lat/alt grid cell indices which loc lies within
    // Returns false if loc is not in grid
    inline bool loc_to_ijk(const Geo3D& loc, IDX3<int>& ijk) const {
        bool inGrid;
        // Get ij
        IDX2<int> ij;
        inGrid = loc_to_ij(loc, ij);
        if (!inGrid) return inGrid;
        // Turn IDX2 object into IDX3
        ijk = ij;
        // Get k
        inGrid = find_k_inside(loc, ijk, ijk.k);
        return inGrid;
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

    bool find_interp_points(const Geo3D& loc, std::vector<IDX3<int>>& interpPoints) const;

    void find_interp_weights(const Geo3D& loc, const std::vector<IDX3<int>>& interpPoints,
                             std::vector<float>& interpWeights) const;

    bool wind_at_loc(const Geo3D& loc, float& u, float& v, float& w) const;
};

#endif