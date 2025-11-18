#ifndef VARIABLES
#define VARIABLES

#include <cstdlib>

class Variable2D {
private:
    float* data;
    int ids, ide, jds, jde;
    int i_size, j_size;
    int num_elements;

public:
    // Constructor
    Variable2D();

    // Destructor
    ~Variable2D();

    void init(int ids, int ide, int jds, int jde);

    size_t get_1D_index_from_2D(int i, int j);

    float* get(int i, int j);
};

#endif