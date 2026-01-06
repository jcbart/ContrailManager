#include <cmath>
#include "thermo.h"

using namespace thermo;

// Returns temperature (K) given potential temperature (K) and total air pressure (Pa)
float thermo::theta_to_T(float theta, float P) {
    return theta * std::pow(P/P0, R_D/c_pd);
}

// Returns specific humidity (kg (kg air)-1) given water vapour mass mixing ratio
// (kg (kg dry air)-1)
float thermo::r_to_q(float r) {
    return r/(1+r);
}

// Returns water vapour partial pressure (Pa) when given water vapour mass mixing ratio
// (kg (kg dry air)-1) and total air pressure (Pa)
float thermo::r_to_e(float r, float P) {
    return (r*P)/(EPS + r);
}

// Returns the satuation vapour pressure with respect to liquid water (Pa) at temperature T (K)
// according to Buck
float thermo::Buck_liq(float T) {
    return 6.1121e2*std::exp((18.678 - (T-273.15)/234.5) * ((T-273.15)/(T-16.01)));
}

// Returns the satuation vapour pressure with respect to ice (Pa) at temperature T (K)
// according to Buck
float thermo::Buck_ice(float T) {
    return 6.1115e2*std::exp((23.036 - (T-273.15)/333.7) * ((T-273.15)/(T+6.67)));
}

// Returns Schumann's G parameter given emissions index of water vapour (EI_H2O; kg (kg fuel)-1),
// total air pressure (P; Pa), specific combustion heat (Q; J kg-1), and propulsion efficiency
// (eta)
float thermo::calc_G(float EI_H2O, float P, float Q, float eta) {
    return (EI_H2O * c_pd * P) / (EPS * Q * (1 - eta));
}

// Initial mass mixing ratio of fuel to total air (approximate)
float thermo::calc_N_initial(float Q, float eta, float T_exhaust, float T_ambient) {
    return Q * (1 - eta) / (c_pd * (T_exhaust - T_ambient));
}

// Approximate water vapour partial pressure at exhaust
float thermo::calc_e_exhaust(float e_amb, float P, float EI_H2O, float N) {
    float m_amb = EPS * e_amb/P;
    float m_p = (EI_H2O + (N - 1) * m_amb) / N;
    return m_p * P / EPS;
}

// Returns the Schumann (1996) estimate of temperature (K) at which the liquid saturation curve is
// tangent to the plume mixing line (Pa K-1)
float thermo::calc_T_LM(float G) {
    float logval = std::log(G-0.053);
    return -ABS_ZERO - 46.46 + 9.43*logval + 0.720*logval*logval;
}

// Returns true if a contrail will form (i.e. mixing line passes liquid water saturation)
// Approximation based on Schumann (1996)
bool thermo::formation_condition_met(float T_exhaust, float e_exhaust, float T_LM, float G) {
    float e_LM = e_exhaust - G * (T_exhaust - T_LM);
    float e_sat_LM = Buck_liq(T_LM);
    bool contrail_forms = false;
    if (e_LM > e_sat_LM) {
        contrail_forms = true;
    }
    return contrail_forms;
}