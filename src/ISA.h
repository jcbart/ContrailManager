#ifndef ISA_H
#define ISA_h

#include <cmath>
#include "constants.h"

namespace ISA {

// Converts pressure altitude (m) to ISA temperature (K)
constexpr double pres_alt_to_T_isa(double alt) {
    double alt_min = std::min(alt, constants::TROPOPAUSE_HEIGHT);
    return constants::T_MSL - alt_min * constants::T_LAPSE_RATE;
}

// Converts pressure altitude (m) within the troposphere to pressure (Pa) using the barometric
// formula
constexpr double troposphere_pres_alt_to_pres(double alt) {
    double T_isa = pres_alt_to_T_isa(alt);
    constexpr double EXPONENT = constants::GRAVITY / (constants::T_LAPSE_RATE * constants::R_D);
    return constants::P_SURFACE * std::pow(T_isa / constants::T_MSL, EXPONENT);
}

// Converts pressure altitude (m) within the stratosphere to pressure (Pa) using the barometric
// formula
constexpr double stratosphere_pres_alt_to_pres(double alt) {
    constexpr double P_TROPOPAUSE_ISA = troposphere_pres_alt_to_pres(constants::TROPOPAUSE_HEIGHT);
    constexpr double T_TROPOPAUSE_ISA = pres_alt_to_T_isa(constants::TROPOPAUSE_HEIGHT);
    double inside_exp = (-constants::GRAVITY / (constants::R_D * T_TROPOPAUSE_ISA))
        * (alt - constants::TROPOPAUSE_HEIGHT);
    return P_TROPOPAUSE_ISA * std::exp(inside_exp);
}

// Converts pressure altitude (m) to pressure (Pa) using the barometric forumla
constexpr double pres_alt_to_pres(double alt) {
    return (alt < constants::TROPOPAUSE_HEIGHT)
        ? troposphere_pres_alt_to_pres(alt) : stratosphere_pres_alt_to_pres(alt);
}

// Converts pressure (Pa) within the troposphere to pressure altitude (m) using the barometric
// formula
constexpr double troposphere_pres_to_pres_alt(double pres) {
    double base = pres / constants::P_SURFACE;
    double exponent = constants::T_LAPSE_RATE * constants::R_D / constants::GRAVITY;
    double T_isa = constants::T_MSL * std::pow(base, exponent);
    return (constants::T_MSL - T_isa) / constants::T_LAPSE_RATE;
}

// Converts pressure (Pa) within the stratosphere to pressure altitude (m) using the barometric
// formula
constexpr double stratosphere_pres_to_pres_alt(double pres) {
    constexpr double P_TROPOPAUSE_ISA = troposphere_pres_alt_to_pres(constants::TROPOPAUSE_HEIGHT);
    constexpr double T_TROPOPAUSE_ISA = pres_alt_to_T_isa(constants::TROPOPAUSE_HEIGHT);
    double inside_exp = std::log(pres / P_TROPOPAUSE_ISA);
    return inside_exp / (-constants::GRAVITY / (constants::R_D * T_TROPOPAUSE_ISA))
        + constants::TROPOPAUSE_HEIGHT;
}

// Converts pressure (Pa) to pressure altitude (m) using the barometric forumla
constexpr double pres_to_pres_alt(double pres) {
    constexpr double P_TROPOPAUSE_ISA = troposphere_pres_alt_to_pres(constants::TROPOPAUSE_HEIGHT);
    return (pres > P_TROPOPAUSE_ISA)
        ? troposphere_pres_to_pres_alt(pres) : stratosphere_pres_to_pres_alt(pres);
}

}

#endif