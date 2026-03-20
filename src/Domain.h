#ifndef DOMAIN_H
#define DOMAIN_H

#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <variant>
#include <algorithm>
#include <ranges>
#include "mapTypes.h"
#include "Projection.h"
#include "CMLog.h"

template <typename T>
class Variable2D {
private:
    std::string name;
    const int ids, ide, jds, jde;
    const int i_size, j_size;
    const size_t num_elements;

    T* data; // Raw, contiguous data

    // Checks indices are valid
    constexpr void check_valid(const int i, const int j) const {
        if (i < ids || i > ide || j < jds || j > jde) [[unlikely]] {
            std::stringstream ss;
            ss << "Variable2D " << name << " error: Index (i,j)=(" << i << "," << j
                << ") is out of range for array of size (ids:ide,jds:jde)=("
                << ids << ":" << ide << "," << jds << ":" << jde << ")";
            CM_RaiseError(ss.str(), __FILE__, __LINE__);
        }
    }

public:
    // Constructor
    Variable2D(std::string name, int ids, int ide, int jds, int jde)
        : name(name), ids(ids), ide(ide), jds(jds), jde(jde),
          i_size(ide - ids + 1), j_size(jde - jds + 1),
          num_elements(i_size * j_size) {
        
        // Allocate a 1D block of memory
        data = new T[num_elements];
        clear_all();
    }

    // Destructor
    ~Variable2D() {
        if (data != nullptr) {
            delete[] data;
        }
    }

    int get_ids() const { return ids; };
    int get_ide() const { return ide; };
    int get_jds() const { return jds; };
    int get_jde() const { return jde; };
    int get_i_size() const { return i_size; };
    int get_j_size() const { return j_size; };
    size_t get_num_elements() const { return num_elements; };
    // Returns a pointer to the first element of data
    // Operations on this pointer are NOT thread-safe
    T* get_data() const { return data; };

    // Flatten 2D indices
    constexpr size_t get_1D_index_from_2D(const int i, const int j) const {
        check_valid(i, j);
        return (static_cast<size_t>(i) - ids) * j_size +
               (static_cast<size_t>(j) - jds);
    }

    // Returns the value at indices; thread-safe
    constexpr T get(const int i, const int j) const {
        T result;
        #pragma omp atomic read
        result = data[get_1D_index_from_2D(i, j)];
        return result;
    }

    // Returns the value at indices; thread-safe
    constexpr T get(const IDX2<int>& ij) const {
        return get(ij.i, ij.j);
    }

    // Returns a pointer to the element at indices
    // Operations on this pointer are NOT thread-safe
    constexpr T* get_ptr(const int i, const int j) const {
        return &data[get_1D_index_from_2D(i, j)];
    }

    // Returns a pointer to the element at indices
    // Operations on this pointer are NOT thread-safe
    constexpr T* get_ptr(const IDX2<int>& ij) const {
        return get_ptr(ij.i, ij.j);
    }

    // Sets the value at indices; thread-safe
    constexpr void set(const int i, const int j, T value) {
        #pragma omp atomic write
        data[get_1D_index_from_2D(i, j)] = value;
    }

    // Sets the value at indices; thread-safe
    constexpr void set(const IDX2<int>& ij, T value) {
        set(ij.i, ij.j, value);
    }

    // Adds to the value at indices; thread-safe
    constexpr void add(const int i, const int j, T value) {
        #pragma omp atomic update
        data[get_1D_index_from_2D(i, j)] += value;
    }

    // Adds to the value at indices; thread-safe
    constexpr void add(const IDX2<int>& ij, T value) {
        add(ij.i, ij.j, value);
    }

    // Subtracts from the value at indices; thread-safe
    constexpr void subtract(const int i, const int j, T value) {
        #pragma omp atomic update
        data[get_1D_index_from_2D(i, j)] -= value;
    }

    // Subtracts from the value at indices; thread-safe
    constexpr void subtract(const IDX2<int>& ij, T value) {
        subtract(ij.i, ij.j, value);
    }

    // Multiplies the value at indices; thread-safe
    template <typename type>
    constexpr void multiply(const int i, const int j, type scalar) {
        #pragma omp atomic update
        data[get_1D_index_from_2D(i, j)] *= scalar;
    }

    // Multiplies the value at indices; thread-safe
    template <typename type>
    constexpr void multiply(const IDX2<int>& ij, type scalar) {
        multiply(ij.i, ij.j, scalar);
    }

    // Divides the value at indices; thread-safe
    template <typename type>
    constexpr void divide(const int i, const int j, type scalar) {
        #pragma omp atomic update
        data[get_1D_index_from_2D(i, j)] /= scalar;
    }

    // Divides the value at indices; thread-safe
    template <typename type>
    constexpr void divide(const IDX2<int>& ij, type scalar) {
        divide(ij.i, ij.j, scalar);
    }

    // Set all values to zero
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

    T* data; // Raw, contiguous data

    // Checks indices are valid
    constexpr void check_valid(const int i, const int j, const int k) const {
        if (i < ids || i > ide || j < jds || j > jde || k < kds || k > kde) [[unlikely]] {
            std::stringstream ss;
            ss << "Variable3D " << name << " error: Index (i,j,k)=(" << i << "," << j << "," << k
                << ") is out of range for array of size (ids:ide,jds:jde,kds:kde)=("
                << ids << ":" << ide << "," << jds << ":" << jde << "," << kds << ":" << kde << ")";
            CM_RaiseError(ss.str(), __FILE__, __LINE__);
        }
    }

public:
    // Constructor
    Variable3D(std::string name, int ids, int ide, int jds, int jde, int kds, int kde)
        : name(name), ids(ids), ide(ide), jds(jds), jde(jde), kds(kds), kde(kde),
          i_size(ide - ids + 1), j_size(jde - jds + 1), k_size(kde - kds + 1),
          num_elements(i_size * j_size * k_size) {
        
        // Allocate a 1D block of memory
        data = new T[num_elements];
        clear_all();
    }
    
    // Destructor
    ~Variable3D() {
        if (data != nullptr) {
            delete[] data;
        }
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
    // Returns a pointer to the first element of data
    // Operations on this pointer are NOT thread-safe
    T* get_data() const { return data; };

    // Flatten 3D indices
    constexpr size_t get_1D_index_from_3D(const int i, const int j, const int k) const {
        check_valid(i, j, k);
        return (static_cast<size_t>(i) - ids) * j_size * k_size +
               (static_cast<size_t>(j) - jds) * k_size +
               (static_cast<size_t>(k) - kds);
    }

    // Returns the value at indices; thread-safe
    constexpr T get(const int i, const int j, const int k) const {
        T result;
        #pragma omp atomic read
        result = data[get_1D_index_from_3D(i, j, k)];
        return result;
    }

    // Returns the value at indices; thread-safe
    constexpr T get(const IDX3<int>& ijk) const {
        return get(ijk.i, ijk.j, ijk.k);
    }

    // Returns a pointer to the element at indices
    // Operations on this pointer are NOT thread-safe
    constexpr T* get_ptr(const int i, const int j, const int k) const {
        return &data[get_1D_index_from_3D(i, j, k)];
    }

    // Returns a pointer to the element at indices
    // Operations on this pointer are NOT thread-safe
    constexpr T* get_ptr(const IDX3<int>& ijk) const {
        return get_ptr(ijk.i, ijk.j, ijk.k);
    }

    // Sets the value at indices; thread-safe
    constexpr void set(const int i, const int j, const int k, T value) {
        #pragma omp atomic write
        data[get_1D_index_from_3D(i, j, k)] = value;
    }

    // Sets the value at indices; thread-safe
    constexpr void set(const IDX3<int>& ijk, T value) {
        set(ijk.i, ijk.j, ijk.k, value);
    }

    // Adds to the value at indices; thread-safe
    constexpr void add(const int i, const int j, const int k, T value) {
        #pragma omp atomic update
        data[get_1D_index_from_3D(i, j, k)] += value;
    }

    // Adds to the value at indices; thread-safe
    constexpr void add(const IDX3<int>& ijk, T value) {
        add(ijk.i, ijk.j, ijk.k, value);
    }

    // Subtracts from the value at indices; thread-safe
    constexpr void subtract(const int i, const int j, const int k, T value) {
        #pragma omp atomic update
        data[get_1D_index_from_3D(i, j, k)] -= value;
    }

    // Subtracts from the value at indices; thread-safe
    constexpr void subtract(const IDX3<int>& ijk, T value) {
        subtract(ijk.i, ijk.j, ijk.k, value);
    }

    // Multiplies the value at indices; thread-safe
    template <typename type>
    constexpr void multiply(const int i, const int j, const int k, type scalar) {
        #pragma omp atomic update
        data[get_1D_index_from_3D(i, j, k)] *= scalar;
    }

    // Multiplies the value at indices; thread-safe
    template <typename type>
    constexpr void multiply(const IDX3<int>& ijk, type scalar) {
        multiply(ijk.i, ijk.j, ijk.k, scalar);
    }

    // Divides the value at indices; thread-safe
    template <typename type>
    constexpr void divide(const int i, const int j, const int k, type scalar) {
        #pragma omp atomic update
        data[get_1D_index_from_3D(i, j, k)] /= scalar;
    }

    // Divides the value at indices; thread-safe
    template <typename type>
    constexpr void divide(const IDX3<int>& ijk, type scalar) {
        divide(ijk.i, ijk.j, ijk.k, scalar);
    }

    // Set all values to zero
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
            QVsave.get_data()[i] = QV.get_data()[i];
        }
    }

    // Update deltaQV with deltaQV = QV - QVsave
    void find_deltaQV() {
        for (size_t i = 0; i < deltaQV.get_num_elements(); i++) {
            deltaQV.get_data()[i] = QV.get_data()[i] - QVsave.get_data()[i];
        }
    }

    // Finds the index k such that loc.alt is inside grid cell ijk
    // Updates k in argument
    // Returns false if no valid k found
    inline bool find_k_inside(const Geo3D& loc, const IDX2<int>& ij, int& k) const {
        // Check if below first or above last boundary
        if (loc.alt < Z_AT_W.get(ij.i, ij.j, kds) ||
            loc.alt >= Z_AT_W.get(ij.i, ij.j, kde + 1)) {
            return false;
        }
        // Binary search within range
        int left = kds, right = kde + 1;
        while (left < right) {
            int mid = left + (right - left) / 2;
            if (loc.alt < Z_AT_W.get(ij.i, ij.j, mid + 1)) {
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
    inline bool find_k_below(const Geo3D& loc, const IDX2<int>& ij, int& k) const {
        // Check if below first or above last centre
        if (loc.alt < Z.get(ij.i, ij.j, kds) ||
            loc.alt >= Z.get(ij.i, ij.j, kde)) {
            return false;
        }
        // Binary search within range
        int left = kds, right = kde;
        while (left < right) {
            int mid = left + (right - left) / 2;
            if (loc.alt < Z.get(ij.i, ij.j, mid + 1)) {
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

    // Returns the lon/lat grid cell indices which loc lies within
    // Return may include fractions depending on its type
    // Calls the method in Domain::proj and removes ids = jds = 0 assumption
    // Exits if loc is not in grid
    // If exiting is not desired, use alternative loc_to_ij method
    template <typename dtype>
    inline IDX2<dtype> loc_to_ij(const Geo2D& loc) const {
        IDX2<dtype> ij;
        if (!loc_to_ij(loc, ij)) [[unlikely]] {
            CM_RaiseUnexpectedOutOfBounds(loc, __FILE__, __LINE__);
        }
        return ij;
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

    // Returns the lon/lat/alt grid cell indices which loc lies within
    // Exits if loc is not in grid
    // If exiting is not desired, use alternative loc_to_ijk method
    inline IDX3<int> loc_to_ijk(const Geo3D& loc) const {
        IDX3<int> ijk;
        if (!loc_to_ijk(loc, ijk)) [[unlikely]] {
            CM_RaiseUnexpectedOutOfBounds(loc, __FILE__, __LINE__);
        }
        return ijk;
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
        loc.lon = XLONG.get(ij);
        loc.lat = XLAT.get(ij);
        return loc;
    }

    // Returns the lat/lon/alt grid values at indices ijk
    inline Geo3D ijk_to_loc(const IDX3<int>& ijk) const {
        Geo3D loc;
        loc.lon = XLONG.get(ijk);
        loc.lat = XLAT.get(ijk);
        loc.alt = Z.get(ijk);
        return loc;
    }

    // Returns true if it is possible to do grid interpolation for loc
    // Currently, this means that loc is not outside the outermost layer of grid cell centres
    inline bool can_do_interp(const Geo3D& loc) const {
        IDX2<int> ij, ijDiag;
        return loc_to_ij_and_diag(loc, ij, ijDiag);
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