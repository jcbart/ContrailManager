#ifndef THERMOUTILS
#define THERMOUTILS

const double R_D = 287; // Gas constant for dry air (J kg-1 K-1)
const double c_pd = 1004; // Specific heat capacity of dry air at constant pressure (J kg-1 K-1)
const double DRY_AIR_MOLAR_MASS = 28.97e-3; // Dry air molar mass (kg mol-1)
const double H2O_MOLAR_MASS = 18.02e-3; // Water molar mass (kg mol-1)
const double ABS_ZERO = -273.15; // Absolute zero in Celcius
const double P0 = 1e5; // Reference pressure (Pa)

float theta_to_T(float T_pot, float P);

float r_to_q(float r);

float r_to_e(float r, float P);

float SAC_T_LM(float G);

#endif