#include <cmath>
#include "thermoUtils.h"

// Returns temperature in Kelvin given potential temperature and total air pressure
float T_pot_to_T(float T_pot, float P) {
    return T_pot * std::pow(P/P0, R_D/c_p);
}