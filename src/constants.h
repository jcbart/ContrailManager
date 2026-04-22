#ifndef CONSTANTS_H
#define CONSTANTS_H

namespace constants {

constexpr double PI = 3.14159265358979323846264338327950288419716939937510582;
constexpr double RAD_PER_DEG = PI/180;
constexpr double DEG_PER_RAD = 1/RAD_PER_DEG;

constexpr double BOLTZMANN_CONSTANT = 1.380649e-23; // Boltzmann's constant (J K-1)
constexpr double AVOGADRO_CONSTANT = 6.02214e23; // Avogadro's constant (mol-1)
constexpr double IDEAL_GAS_CONSTANT = 8.3145e0; // Ideal gas constant (J mol-1 K-1)
constexpr double R_D = 287.05; // Gas constant for dry air (J kg-1 K-1)
constexpr double c_pd = 1004; // Specific heat capacity of dry air at constant pressure (J kg-1 K-1)
constexpr double c_pv = 1870; // Specific heat capacity of water vapor (J kg-1 K-1)
constexpr double DRY_AIR_MOLAR_MASS = 28.97e-3; // Molar mass of dry air (kg mol-1)
constexpr double H2O_MOLAR_MASS = 18.02e-3; // Molar mass of H2O (kg mol-1)
constexpr double H2O_MOLECULAR_MASS = 2.991506e-26; // Mass of an H2O molecule (kg)
constexpr double ABS_ZERO = -273.15; // Absolute zero in Celcius
constexpr double P0 = 1e5; // Reference pressure (Pa)
constexpr double EPS = H2O_MOLAR_MASS / DRY_AIR_MOLAR_MASS; // Ratio of water to dry air molar mass
// Ratio of specific heat capacity at constant pressure to that at constant volume for a diatomic ideal gas
constexpr double GAMMA = 1.4;

constexpr double EARTH_RADIUS_M = 6.37e6; // Earth radius (m); consistent with WRF
constexpr double GRAVITY = 9.80665; // Acceleration due to gravity (m s-2)

}

#endif