#ifndef VARIABLES
#define VARIABLES

#include <string>
#include <vector>

class Variable2D {
private:
    std::string name = "UNDEFINED";
    float* data;
    int ids, ide, jds, jde = 0;
    int i_size, j_size = 0;
    int num_elements = 0;
    bool isInitialised = false;

public:
    // Constructor
    Variable2D();

    // Destructor
    ~Variable2D();

    void init(std::string name, int ids, int ide, int jds, int jde);

    int get_ids() {return ids;};
    int get_ide() {return ide;};
    int get_jds() {return jds;};
    int get_jde() {return jde;};
    int get_i_size() {return i_size;};
    int get_j_size() {return j_size;};

    size_t get_1D_index_from_2D(int i, int j);

    float* get(int i, int j);
};

class Variable3D {
private:
    std::string name = "UNDEFINED";
    float* data;
    int ids, ide, jds, jde, kds, kde = 0;
    int i_size, j_size, k_size = 0;
    int num_elements = 0;
    bool isInitialised = false;

public:
    // Constructor
    Variable3D();

    // Destructor
    ~Variable3D();

    void init(std::string name, int ids, int ide, int jds, int jde, int kds, int kde);

    int get_ids() {return ids;};
    int get_ide() {return ide;};
    int get_jds() {return jds;};
    int get_jde() {return jde;};
    int get_kds() {return kds;};
    int get_kde() {return kde;};
    int get_i_size() {return i_size;};
    int get_j_size() {return j_size;};
    int get_k_size() {return k_size;};

    size_t get_1D_index_from_3D(int i, int j, int k);

    float* get(int i, int j, int k);
};

#endif