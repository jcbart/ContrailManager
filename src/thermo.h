#ifndef THERMO_H
#define THERMO_H

#include <cmath>
#include <algorithm>
#include "constants.h"

namespace thermo {

// Returns air density (kg m-3) for (T, P) assuming dry air
constexpr double rho_d(double T, double P) {
    return (P / (constants::R_D * T));
}

// Returns temperature (K) given potential temperature (K) and air pressure (Pa)
constexpr double theta_to_T(double theta, double P) {
    return theta * std::pow(P / constants::P0, constants::R_D / constants::c_pd);
}

// Returns potential temperature (K) given temperature (K) and air pressure (Pa)
constexpr double T_to_theta(double T, double P) {
    return T * std::pow(constants::P0 / P, constants::R_D / constants::c_pd);
}

// Returns specific humidity (kg (kg air)-1) given water vapour mass mixing ratio
// (kg (kg dry air)-1)
constexpr double r_to_q(double r) {
    return r / (1 + r);
}

// Returns water vapour mass mixing ratio (kg (kg dry air)-1) given specific humidity
// (kg (kg air)-1) 
constexpr double q_to_r(double q) {
    return q / (1 - q);
}

// Returns water vapour partial pressure (Pa) when given water vapour mass mixing ratio
// (kg (kg dry air)-1) and total air pressure (Pa)
constexpr double r_to_e(double r, double P) {
    return (r * P) / (constants::EPS + r);
}

// Returns water vapour mass mixing ratio (kg (kg dry air)-1) when given water vapour partial
// pressure (Pa) and total air pressure (Pa)
constexpr double e_to_r(double e, double P) {
    return (constants::EPS * e) / (P - e);
}

// Returns saturation pressure of water vapor over ice (Pa) given T (K) according to Sonntag (1994)
constexpr double e_sat_ice(double T) {
    return (100.0 * std::exp(
        (-6024.5282 / T)
        + 24.7219
        + (0.010613868 * T)
        - (1.3198825e-5 * (T*T))
        - 0.49382577 * std::log(T)
    ));
}

// Returns saturation water vapor pressure with respect to liquid water (Pa) given T (K)
// according to Murphy and Koop (2005)
constexpr double e_sat_liquid(double T) {
    return std::exp(
        54.842763
        - 6763.22 / T
        - 4.21 * std::log(T)
        + 0.000367 * T
        + std::tanh(0.0415 * (T - 218.8))
        * (53.878 - 1331.22 / T - 9.44523 * std::log(T) + 0.014025 * T)
    );
}

// Returns the derivative of saturation water vapor pressure with respect to liquid water
// (Pa K-1) given T (K)
constexpr double e_sat_liquid_prime(double T) {
    double tanh_term = std::tanh(0.0415 * (T - 218.8));
    return e_sat_liquid(T) * (
        6763.22 / (T*T)
        - 4.21 / T
        + 0.000367
        + 0.0415 * (1 - tanh_term*tanh_term) * (
            53.878 - 1331.22 / T - 9.44523 * std::log(T) + 0.014025 * T
        )
        + tanh_term * (1331.22 / (T*T) - 9.44523 / T + 0.014025)
    );
}

// Returns saturation specific humidity over ice (kg (kg air)-1)
constexpr double q_sat_ice(double T, double P) {
    return (constants::EPS * e_sat_ice(T) / P);
}

// Returns saturation specific humidity over liquid water (kg (kg air)-1)
constexpr double q_sat_liquid(double T, double P) {
    return (constants::EPS * e_sat_liquid(T) / P);
}

// Returns specific latent heat of sublimation from and of deposition to ice (J kg-1)
constexpr double slh_sublimation_ice(double T) {
    const double T_C = T + constants::ABS_ZERO;
    return 1e3 * (2834.1 - 0.29 * T_C - 4e-3 * T_C * T_C);
}

// Returns isobaric heat capacity of moist air (J kg-1 K-1) given specific humidity (kg (kg air)-1)
constexpr double c_pm(double q) {
    return (constants::c_pd * (1 - q) + constants::c_pv * q);
}

// Returns the potential temperature gradient (K m-1) between two altitudes
constexpr double theta_gradient(double T_top, double P_top, double T_btm,
    double P_btm, double dz) {
    
    return (T_to_theta(T_top, P_top) - T_to_theta(T_btm, P_btm)) / dz;
}

// Calculate the Brunt-Vaisala frequency (s-1) where dtheta_dz is potential temperature gradient
constexpr double brunt_vaisala_frequency(double P, double T, double dtheta_dz) {
    return std::sqrt(std::max(1e-6, dtheta_dz) * constants::GRAVITY / T_to_theta(T, P));
}

}

#endif