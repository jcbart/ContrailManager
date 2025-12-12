#include <iostream>
#include <string>
#include "variables.h"
#include "mapUtils.h"

// ---------- Variable2D ----------

// Constructor
Variable2D::Variable2D() {
    data = nullptr;
    ids = 0;
    ide = 0;
    jds = 0;
    jde = 0;
    i_size = 0;
    j_size = 0;
    num_elements = 0;
    isInitialised = false;
}

// Destructor
Variable2D::~Variable2D() {
    if (data != nullptr) {
        delete[] data;
    }
}

void Variable2D::init(std::string name, int ids, int ide, int jds, int jde) {
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
    data = new float[num_elements];
    clear_all();
    isInitialised = true;
}

size_t Variable2D::get_1D_index_from_2D(int i, int j) {
    return static_cast<size_t>((i-ids)*j_size + (j-jds));
}

// Returns a reference to the value, so can be used to set and get
float* Variable2D::get(const int i, const int j) {
    if (i < ids || i > ide || j < jds || j > jde) {
        std::cerr << "Variable2D " << name << " error: Index (i,j)=(" << i << "," << j <<
                     ") is out of range for array of size (ids:ide,jds:jde)=(" <<
                     ids << ":" << ide << "," << jds << ":" << jde << ")" << std::endl;
        exit(EXIT_FAILURE);
        return &data[get_1D_index_from_2D(ids, jds)];
    }
    else {
        return &data[get_1D_index_from_2D(i, j)];
    }
}

// Returns a reference to the value, so can be used to set and get
float* Variable2D::get(const IDX2& ij) {
    return get(ij.i, ij.j);
}

// Set all values to zero; only works if initialised
void Variable2D::clear_all() {
    if (isInitialised) {
        for (size_t i = 0; i < num_elements; i++) {
            data[i] = 0;
        }
    }
}

// ---------- Variable3D ----------

// Constructor
Variable3D::Variable3D() {
    data = nullptr;
    ids = 0;
    ide = 0;
    jds = 0;
    jde = 0;
    kds = 0;
    kde = 0;
    i_size = 0;
    j_size = 0;
    k_size = 0;
    num_elements = 0;
    isInitialised = false;
}

// Destructor
Variable3D::~Variable3D() {
    if (data != nullptr) {
        delete[] data;
    }
}

void Variable3D::init(std::string name, int ids, int ide, int jds, int jde, int kds, int kde) {
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
    data = new float[num_elements];
    clear_all();
    isInitialised = true;
}

size_t Variable3D::get_1D_index_from_3D(int i, int j, int k) {
    return static_cast<size_t>((i-ids)*j_size*k_size + (j-jds)*k_size + (k-kds));
}

// Returns a reference to the value, so can be used to set and get
float* Variable3D::get(const int i, const int j, const int k) {
    if (i < ids || i > ide || j < jds || j > jde || k < kds || k > kde) {
        std::cerr << "Variable3D " << name << " error: Index (i,j,k)=(" << i << "," << j << "," << k <<
                     ") is out of range for array of size (ids:ide,jds:jde,kds:kde)=("
                     << ids << ":" << ide << "," << jds << ":" << jde << "," << kds << ":" << kde << ")"
                     << std::endl;
        exit(EXIT_FAILURE);
        return &data[get_1D_index_from_3D(ids, jds, kds)];
    }
    else {
        return &data[get_1D_index_from_3D(i, j, k)];
    }
}

// Returns a reference to the value, so can be used to set and get
float* Variable3D::get(const IDX3& ijk) {
    return get(ijk.i, ijk.j, ijk.k);
}

// Set all values to zero; only works if initialised
void Variable3D::clear_all() {
    if (isInitialised) {
        for (size_t i = 0; i < num_elements; i++) {
            data[i] = 0;
        }
    }
}