#include <iostream>
#include <string>
#include <vector>
#include <ESMC.h>
#include "Domain.h"
#include "mapUtils.h"

// List of types to compile
template class Variable2D<float>;
template class Variable3D<float>;

// ---------- Variable2D ----------

// Destructor
template <typename T>
Variable2D<T>::~Variable2D() {
    if (data != nullptr) {
        delete[] data;
    }
}

template <typename T>
void Variable2D<T>::init(std::string name, int ids, int ide, int jds, int jde) {
    if (isInitialised) {
        std::cerr << "Variable2D " << this->name << " has already been initialised" << std::endl;
        exit(EXIT_FAILURE);
    }
    this->name = name;
    this->ids = ids;
    this->ide = ide;
    this->jds = jds;
    this->jde = jde;
    i_size = ide-ids+1;
    j_size = jde-jds+1;
    num_elements = i_size*j_size;
    // Allocate a 1D block of memory
    data = new T[num_elements];
    clear_all();
    isInitialised = true;
}

template <typename T>
size_t Variable2D<T>::get_1D_index_from_2D(int i, int j) const {
    return static_cast<size_t>((i-ids)*j_size + (j-jds));
}

template <typename T>
const T* Variable2D<T>::get_element_ptr(const int i, const int j) const {
    if (i < ids || i > ide || j < jds || j > jde) {
        std::cerr << "Variable2D " << name << " error: Index (i,j)=(" << i << "," << j <<
                     ") is out of range for array of size (ids:ide,jds:jde)=(" <<
                     ids << ":" << ide << "," << jds << ":" << jde << ")" << std::endl;
        exit(EXIT_FAILURE);
    }
    return &data[get_1D_index_from_2D(i, j)];
}

// Returns a reference to the value, so can be used to set and get
template <typename T>
T* Variable2D<T>::get(const int i, const int j) {
    return const_cast<T*>(get_element_ptr(i, j));
}

// Returns a reference to the value, so can be used to set and get
template <typename T>
T* Variable2D<T>::get(const IDX2<int>& ij) {
    return get(ij.i, ij.j);
}

// Returns the value at indices
template <typename T>
T Variable2D<T>::get_value(const int i, const int j) const {
    return *get_element_ptr(i, j);
}

// Returns the value at indices
template <typename T>
T Variable2D<T>::get_value(const IDX2<int>& ij) const {
    return get_value(ij.i, ij.j);
}

// Set all values to zero; only works if initialised
template <typename T>
void Variable2D<T>::clear_all() {
    if (isInitialised) {
        for (size_t i = 0; i < num_elements; i++) {
            data[i] = 0;
        }
    }
}

// ---------- Variable3D ----------

// Destructor
template <typename T>
Variable3D<T>::~Variable3D() {
    if (data != nullptr) {
        delete[] data;
    }
}

template <typename T>
void Variable3D<T>::init(std::string name, int ids, int ide, int jds, int jde, int kds, int kde) {
    if (isInitialised) {
        std::cerr << "Variable3D " << this->name << " has already been initialised" << std::endl;
        exit(EXIT_FAILURE);
    }
    this->name = name;
    this->ids = ids;
    this->ide = ide;
    this->jds = jds;
    this->jde = jde;
    this->kds = kds;
    this->kde = kde;
    i_size = ide-ids+1;
    j_size = jde-jds+1;
    k_size = kde-kds+1;
    num_elements = i_size*j_size*k_size;
    // Allocate a 1D block of memory
    data = new T[num_elements];
    clear_all();
    isInitialised = true;
}

template <typename T>
size_t Variable3D<T>::get_1D_index_from_3D(int i, int j, int k) const {
    return static_cast<size_t>((i-ids)*j_size*k_size + (j-jds)*k_size + (k-kds));
}

template <typename T>
const T* Variable3D<T>::get_element_ptr(const int i, const int j, const int k) const {
    if (i < ids || i > ide || j < jds || j > jde || k < kds || k > kde) {
        std::cerr << "Variable3D " << name << " error: Index (i,j,k)=(" << i << "," << j << "," << k <<
                     ") is out of range for array of size (ids:ide,jds:jde,kds:kde)=("
                     << ids << ":" << ide << "," << jds << ":" << jde << "," << kds << ":" << kde << ")"
                     << std::endl;
        exit(EXIT_FAILURE);
    }
    return &data[get_1D_index_from_3D(i, j, k)];
}

// Returns a reference to the value, so can be used to set and get
template <typename T>
T* Variable3D<T>::get(const int i, const int j, const int k) {
    return const_cast<T*>(get_element_ptr(i, j, k));
}

// Returns a reference to the value, so can be used to set and get
template <typename T>
T* Variable3D<T>::get(const IDX3<int>& ijk) {
    return get(ijk.i, ijk.j, ijk.k);
}

// Returns the value at indices
template <typename T>
T Variable3D<T>::get_value(const int i, const int j, const int k) const {
    return *get_element_ptr(i, j, k);
}

// Returns the value at indices
template <typename T>
T Variable3D<T>::get_value(const IDX3<int>& ijk) const {
    return get_value(ijk.i, ijk.j, ijk.k);
}

// Set all values to zero; only works if initialised
template <typename T>
void Variable3D<T>::clear_all() {
    if (isInitialised) {
        for (size_t i = 0; i < num_elements; i++) {
            data[i] = 0;
        }
    }
}

// ---------- Domain ----------

void Domain::init_vars(int ids, int ide, int jds, int jde, int kds, int kde) {
    int rc;
    std::string msg;

    if (varsInitd) {
        std::cerr << "Contrail Manager domain error: init_vars() called when variables already "
                  << "initialised. Stopping." << std::endl;
        exit(EXIT_FAILURE);
    }

    XLONG.init("XLONG", ids, ide, jds, jde);
    XLAT.init("XLAT", ids, ide, jds, jde);
    Z.init("Z", ids, ide, jds, jde, kds, kde);
    Z_AT_W.init("Z_AT_W", ids, ide, jds, jde, kds, kde+1);
    DRYMASS.init("DRYMASS", ids, ide, jds, jde, kds, kde);
    T_POT.init("T_POT", ids, ide, jds, jde, kds, kde);
    P.init("P", ids, ide, jds, jde, kds, kde);
    U.init("U", ids, ide, jds, jde, kds, kde);
    V.init("V", ids, ide, jds, jde, kds, kde);
    W.init("W", ids, ide, jds, jde, kds, kde);
    TNSR.init("TNSR", ids, ide, jds, jde);
    OLR.init("OLR", ids, ide, jds, jde);
    QV.init("QV", ids, ide, jds, jde, kds, kde);
    QVsave.init("QV", ids, ide, jds, jde, kds, kde);
    deltaQV.init("deltaQV", ids, ide, jds, jde, kds, kde);
    QI.init("QI", ids, ide, jds, jde, kds, kde);
    deltaQI.init("deltaQI", ids, ide, jds, jde, kds, kde);
    deltaNI.init("deltaNI", ids, ide, jds, jde, kds, kde);
    QIcontrail.init("QIcontrail", ids, ide, jds, jde, kds, kde);
    
    varsInitd = true;

    // Take sizes from Z
    this->ids = Z.get_ids();
    this->ide = Z.get_ide();
    this->jds = Z.get_jds();
    this->jde = Z.get_jde();
    this->kds = Z.get_kds();
    this->kde = Z.get_kde();
    lonSize = Z.get_i_size();
    latSize = Z.get_j_size();
    altSize = Z.get_k_size();

    msg = "Contrail Manager variables initialised with dimensions:";
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
    msg = "ids = " + std::to_string(ids) + ", jds = " + std::to_string(jds) + ", kds = " + std::to_string(kds);
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
    msg = "ide = " + std::to_string(ide) + ", jde = " + std::to_string(jde) + ", kde = " + std::to_string(kde);
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
    msg = "Note: Contrail Manager does not use a staggered grid except for Z_AT_W where kde += 1.";
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
}

// Copy contents of QV to QVsave
void Domain::save_QV() {
    for (size_t i = 0; i < QV.get_num_elements(); i++) {
        QVsave.data[i] = QV.data[i];
    }
}

// Update deltaQV with deltaQV = QV - QVsave
void Domain::find_deltaQV() {
    for (size_t i = 0; i < deltaQV.get_num_elements(); i++) {
        deltaQV.data[i] = QV.data[i] - QVsave.data[i];
    }
}

// Finds interpolation points for a location and updates interpPoints
// Returns true if location is in grid
// If false, interp contains garbage
bool Domain::find_interp_points(const Geo3D& loc, std::vector<IDX3<int>>& interpPoints) const {
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
void Domain::find_interp_weights(const Geo3D& loc, const std::vector<IDX3<int>>& interpPoints,
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
bool Domain::wind_at_loc(const Geo3D& loc, float& u, float& v, float& w) const {
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