#include <cstdlib>
#include <iostream>
#include "variables.h"

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
}

// Destructor
Variable2D::~Variable2D() {
    if (data != nullptr) {
        delete[] data;
    }
}

void Variable2D::init(int ids, int ide, int jds, int jde) {
    this->ids = ids;
    this->ide = ide;
    this->jds = jds;
    this->jde = jde;
    i_size = ide-ids+1;
    j_size = jde-jds+1;
    num_elements = i_size*j_size;
    // Allocate a 1D block of memory
    data = new float[num_elements];
    for (int i = 0; i < num_elements; i++) {
        data[i] = 0;
    }
}

size_t Variable2D::get_1D_index_from_2D(int i, int j) {
    return static_cast<size_t>((j-jds)*i_size + (i-ids));
}

// Returns a reference to the value, so can be used to set and get
float* Variable2D::get(int i, int j) {
    if (i < ids || i > ide || j < jds || j > jde) {
        //*rc = ESMF_RC_VAL_OUTOFRANGE;
        std::cerr << "Error: Index (i,j)=(" << i << "," << j << ") is out of range for array of size"
                     "(ids:ide, jds:jde)=(" << ids << ":" << ide << "," << jds << ":" << jde << ")"
                     << std::endl;
        exit(EXIT_FAILURE);
        return &data[get_1D_index_from_2D(ids, jds)];
    }
    else {
        //*rc = ESMF_SUCCESS;
        return &data[get_1D_index_from_2D(i, j)];
    }
}

