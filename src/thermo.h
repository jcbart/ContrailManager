#ifndef THERMO_H
#define THERMO_H

namespace thermo {

const double BOLTZMANN_CONSTANT = 1.380649e-23; // Boltzmann's constant (J K-1)
const double AVOGADRO_CONSTANT = 6.02214e23; // Avogadro's constant (mol-1)
const double IDEAL_GAS_CONSTANT = 8.3145e0; // Ideal gas constant (J mol-1 K-1)
const double R_D = 287; // Gas constant for dry air (J kg-1 K-1)
const double c_pd = 1004; // Specific heat capacity of dry air at constant pressure (J kg-1 K-1)
const double DRY_AIR_MOLAR_MASS = 28.97e-3; // Molar mass of dry air (kg mol-1)
const double H2O_MOLAR_MASS = 18.02e-3; // Molar mass of H2O (kg mol-1)
const double H2O_MOLECULAR_MASS = 2.991506e-26; // Mass of an H2O molecule (kg)
const double ABS_ZERO = -273.15; // Absolute zero in Celcius
const double P0 = 1e5; // Reference pressure (Pa)
const double EPS = H2O_MOLAR_MASS/DRY_AIR_MOLAR_MASS; // Ratio of water to dry air molar mass

float theta_to_T(float T_pot, float P);

float r_to_q(float r);

float r_to_e(float r, float P);

float e_to_r(float e, float P);

float Buck_liq(float T);

float Buck_ice(float T);

float calc_G(float EI_H2O, float P, float Q, float eta);

float calc_N_initial(float Q, float eta, float T_exhaust, float T_ambient);

float calc_e_exhaust(float e_amb, float P, float EI_H2O, float N);

float calc_T_LM(float G);

bool formation_condition_met(float T_exhaust, float e_exhaust, float T_LM, float G);

}

#endif