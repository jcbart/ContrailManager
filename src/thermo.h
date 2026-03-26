#ifndef THERMO_H
#define THERMO_H

#include <cmath>

namespace thermo {

constexpr double BOLTZMANN_CONSTANT = 1.380649e-23; // Boltzmann's constant (J K-1)
constexpr double AVOGADRO_CONSTANT = 6.02214e23; // Avogadro's constant (mol-1)
constexpr double IDEAL_GAS_CONSTANT = 8.3145e0; // Ideal gas constant (J mol-1 K-1)
constexpr double R_D = 287.05; // Gas constant for dry air (J kg-1 K-1)
constexpr double c_pd = 1004; // Specific heat capacity of dry air at constant pressure (J kg-1 K-1)
constexpr double DRY_AIR_MOLAR_MASS = 28.97e-3; // Molar mass of dry air (kg mol-1)
constexpr double H2O_MOLAR_MASS = 18.02e-3; // Molar mass of H2O (kg mol-1)
constexpr double H2O_MOLECULAR_MASS = 2.991506e-26; // Mass of an H2O molecule (kg)
constexpr double ABS_ZERO = -273.15; // Absolute zero in Celcius
constexpr double P0 = 1e5; // Reference pressure (Pa)
constexpr double EPS = H2O_MOLAR_MASS/DRY_AIR_MOLAR_MASS; // Ratio of water to dry air molar mass

// Returns air density (kg m-3) for (T, P) assuming dry air
constexpr double rho_d(double T, double P) {
    return (P / (R_D * T));
}

// Returns temperature (K) given potential temperature (K) and air pressure (Pa)
constexpr double theta_to_T(double theta, double P) {
    return theta * std::pow(P / P0, R_D / c_pd);
}

// Returns potential temperature (K) given temperature (K) and air pressure (Pa)
constexpr double T_to_theta(double T, double P) {
    return T * std::pow(P0 / P, R_D / c_pd);
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
    return (r * P) / (EPS + r);
}

// Returns water vapour mass mixing ratio (kg (kg dry air)-1) when given water vapour partial
// pressure (Pa) and total air pressure (Pa)
constexpr double e_to_r(double e, double P) {
    return (EPS * e) / (P - e);
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

// Returns saturation specific humidity over ice (kg (kg air)-1)
constexpr double q_sat_ice(double T, double P) {
    return (EPS * e_sat_ice(T) / P);
}

// Returns specific latent heat of sublimation from and of deposition to ice (J kg-1)
constexpr double slh_sublimation_ice(double T) {
    return 1e3 * (2834.1 - 0.29 * (T + ABS_ZERO) - 4e-3 * (T + ABS_ZERO) * (T + ABS_ZERO));
}

}

#endif