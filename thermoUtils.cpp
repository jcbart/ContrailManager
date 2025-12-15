#include <cmath>
#include "thermoUtils.h"

// Returns temperature (K) given potential temperature (K) and total air pressure (Pa)
float theta_to_T(float theta, float P) {
    return theta * std::pow(P/P0, R_D/c_pd);
}

// Returns specific humidity (kg (kg air)-1) given water vapour mass mixing ratio
// (kg (kg dry air)-1)
float r_to_q(float r) {
    return r/(1+r);
}

// Returns water vapour partial pressure (Pa) when given water vapour mass mixing ratio
// (kg (kg dry air)-1) and total air pressure (Pa)
float r_to_e(float r, float P) {
    return (r*P)/(H2O_MOLAR_MASS/DRY_AIR_MOLAR_MASS + r);
}

// Returns the Schumann (1996) estimate of temperature (K) at which the liquid saturation curve is
// tangent to the plume mixing line (Pa K-1)
float SAC_T_LM(float G) {
    float logval = log(G-0.053);
    return -ABS_ZERO - 46.46 + 9.43*logval + 0.720*logval*logval;
}