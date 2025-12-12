#ifndef THERMOUTILS
#define THERMOUTILS

const double R_D = 287; // Gas constant for dry air (J kg-1 K-1)
const double c_p = 1004; // Specific heat capacity of dry air at constant pressure (J kg-1 K-1)
const double P0 = 1e5; // Reference pressure (Pa)

float T_pot_to_T(float T_pot, float P);

#endif