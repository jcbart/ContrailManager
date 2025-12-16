#ifndef VARIABLES
#define VARIABLES

#include <string>
#include <vector>
#include "mapUtils.h"

class Variable2D {
private:
    std::string name = "UNDEFINED";
    float* data;
    int ids = 0, ide = 0, jds = 0, jde = 0;
    int i_size = 0, j_size = 0;
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

    float* get(const int i, const int j);
    float* get(const IDX2& ij);

    void clear_all();
};

class Variable3D {
private:
    std::string name = "UNDEFINED";
    float* data;
    int ids = 0, ide = 0, jds = 0, jde = 0, kds = 0, kde = 0;
    int i_size = 0, j_size = 0, k_size = 0;
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

    float* get(const int i, const int j, const int k);
    float* get(const IDX3& ijk);

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
    // Meteorological variables (accessible externally)
    Variable2D XLONG; // Longitude (degrees, West is negative)
    Variable2D XLAT; // Latitude (degrees, South is negative)
    Variable3D Z; // Height above sea level at cell centre (m)
    Variable3D Z_AT_W; // Height above sea level at cell interfaces (staggered in z-direction; m)
    Variable3D DRYMASS; // Dry mass in grid cell (kg)
    Variable3D T_POT; // Potential temperature (K)
    Variable3D P; // Total air pressure (Pa)
    Variable3D U; // Wind speed in Eastward direction (m s-1)
    Variable3D V; // Wind speed in Northward direction (m s-1)
    Variable3D W; // Wind speed in vertical direction (m s-1)
    Variable3D QV; // Water vapour mass mixing ratio (kg (kg dry air-1))
    Variable3D deltaQV; // Change in water vapour mass mixing ratio (kg (kg dry air-1))
    //Variable3D QI; // Ice mass mixing ratio excl. live contrails (kg (kg dry air-1))
    Variable3D deltaQI; // Change in ice mass mixing ratio excl. live contrails (kg (kg dry air-1))
    //Variable3D NI; // Ice number mixing ratio excl. live contrails (# (kg dry air-1))
    Variable3D deltaNI; // Change in ice number mixing ratio excl. live contrails (# (kg dry air-1))
    Variable3D QIcontrail; // Contrail ice mass mixing ratio (kg (kg dry air-1))

    int get_ids() {return ids;}
    int get_ide() {return ide;}
    int get_jds() {return jds;}
    int get_jde() {return jde;}
    int get_kds() {return kds;}
    int get_kde() {return kde;}
    int get_lonSize() {return lonSize;};
    int get_latSize() {return latSize;};
    int get_altSize() {return altSize;};
    bool get_varsInitd() {return varsInitd;}

    void init_vars(int ids, int ide, int jds, int jde, int kds, int kde);
};

#endif