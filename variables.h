#ifndef VARIABLES
#define VARIABLES

#include <vector>

class Variable2D {
private:
    //float* data;
    std::vector<float> data;
    int ids, ide, jds, jde;
    int i_size, j_size;
    int num_elements;
    bool isInitialised;

public:
    // Constructor
    Variable2D();

    // Destructor
    ~Variable2D();

    void init(int ids, int ide, int jds, int jde);

    int get_i_size() {return i_size;};
    int get_j_size() {return j_size;};

    size_t get_1D_index_from_2D(int i, int j);

    float* get(int i, int j);
};

class Variable3D {
private:
    //float* data;
    std::vector<float> data;
    int ids, ide, jds, jde, kds, kde;
    int i_size, j_size, k_size;
    int num_elements;
    bool isInitialised;

public:
    // Constructor
    Variable3D();

    // Destructor
    ~Variable3D();

    void init(int ids, int ide, int jds, int jde, int kds, int kde);

    int get_i_size() {return i_size;};
    int get_j_size() {return j_size;};
    int get_k_size() {return k_size;};

    size_t get_1D_index_from_3D(int i, int j, int k);

    float* get(int i, int j, int k);
};

#endif