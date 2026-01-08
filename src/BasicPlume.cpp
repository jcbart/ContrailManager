#include <ESMC.h>
#include <cmath>
#include "plumeModels.h"
#include "domain.h"
#include "timekeeping.h"
#include "thermo.h"
#include "mathUtils.h"

using namespace mathUtils;

// Constants
const double BOLTZMANN_CONSTANT = 1.380649e-23; // (J K-1)
const double H2O_MOLECULAR_MASS = 2.991506e-26; // (kg)
const double AVOGADRO_CONSTANT = 6.02214e23; // (mol-1)
const double IDEAL_GAS_CONSTANT = 8.3145e0; // (J mol-1 K-1)
const double ICE_DENSITY = 917; // Approx assumption (kg m-3)
const double H2O_VOL_ICE = H2O_MOLECULAR_MASS / ICE_DENSITY; // (m3)

// Parameters
const float MIN_M_ICE_PER_M = 0.01; // kg m-1

void SegmentBasicPlume::integrate(const CMTime& timeStepStart, const CMTime& timeStepEnd) {
    if (!doneFormation) {
        formation();
        doneFormation = true;
        if (isDead) {
            return;
        }
    }

    int duration_s = (timeStepEnd - timeStepStart).dhms_to_s();

    IDX3 ijkCurr;
    // Should be safe to ignore return value
    bool inGrid = domPtr->loc_to_ijk(centre, ijkCurr);

    const float accom_coeff = 1;

    const float P_amb = domPtr->P.get_value(ijkCurr);
    const float T_amb = thermo::theta_to_T(domPtr->T_POT.get_value(ijkCurr), P_amb);
    float qv_amb = domPtr->QV.get_value(ijkCurr);
    float e_amb = thermo::r_to_e(qv_amb, P_amb);
    const float S_i = e_amb/thermo::Buck_ice(T_amb);

    float air_diffusivity = 2.11e-5 * std::pow(T_amb/273.15, 1.94) * (101325 / P_amb);
    float vapour_thermal_speed = std::sqrt(8 * BOLTZMANN_CONSTANT * T_amb / (PI*H2O_MOLECULAR_MASS));
    float n_sat = AVOGADRO_CONSTANT * e_amb / (IDEAL_GAS_CONSTANT * T_amb);

    float correction_factor = 1 + accom_coeff * vapour_thermal_speed * r_ice / (4 * air_diffusivity);
    // Flux of water vapour molecules to a crystal (s-1)
    float J = (PI * r_ice * r_ice * accom_coeff * vapour_thermal_speed * n_sat) / correction_factor * (S_i - 1);
    // Growth rate of a crystal (m3 s-1)
    float dv_dt = H2O_VOL_ICE * J;
    // Change in volume of a crystal (m3)
    float dv_single = dv_dt * duration_s;
    // Change in ambient vapour pressure
    float delta_e_amb = -(BOLTZMANN_CONSTANT * T_amb * dv_single*n_ice/H2O_VOL_ICE);

    // Update crystal size
    float r_ice = v_to_r(r_to_v(r_ice) + dv_single);

    // Update mass per metre
    M_ice_per_m = r_to_v(r_ice) * ICE_DENSITY * N_ice_per_m;

    if (domPtr->onlineCoupling) {
        // Update ambient water vapour
        e_amb += delta_e_amb;
        float qv_amb_new = thermo::e_to_r(e_amb, P_amb);
        float delta_qv = qv_amb_new - qv_amb;
        *domPtr->QV.get(ijkCurr) += delta_qv;
    }

    if (M_ice_per_m < MIN_M_ICE_PER_M) {
        isDead = true;
    }
}

void SegmentBasicPlume::dump() {
    // Only really need to do this if formation happened
    IDX3 ijkCurr;
    // Should be safe to ignore return value
    bool inGrid = domPtr->loc_to_ijk(centre, ijkCurr);
    float grid_dry_mass = domPtr->DRYMASS.get_value(ijkCurr);
    // Ice mass
    float M_ice = M_ice_per_m*length;
    *domPtr->deltaQI.get(ijkCurr) += M_ice/grid_dry_mass;
    // Ice number
    float N_ice = N_ice_per_m*length;
    *domPtr->deltaNI.get(ijkCurr) += N_ice/grid_dry_mass;
}

void SegmentBasicPlume::addToQIcontrail() {
    IDX3 ijkCurr;
    // Should be safe to ignore return value
    bool inGrid = domPtr->loc_to_ijk(centre, ijkCurr);
    float m_ice = M_ice_per_m*length;
    float grid_dry_mass = domPtr->DRYMASS.get_value(ijkCurr);
    *domPtr->QIcontrail.get(ijkCurr) += m_ice/grid_dry_mass;
    int rc;
    std::string msg;
    msg = "QIcontrail at " + ijkCurr.asString() + " set to " + std::to_string(*domPtr->QIcontrail.get(ijkCurr));
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
}

void SegmentBasicPlume::scaleWidthAfterAdvection(float lengthRatio) {
    cross_section_area /= lengthRatio;
    N_ice_per_m = n_ice * cross_section_area; 
}

void SegmentBasicPlume::formation() {
    IDX3 ijkCurr;
    // Should be safe to ignore return value
    bool inGrid = domPtr->loc_to_ijk(centre, ijkCurr);
    float T_exhaust = 600;
    float EI_H2O = 1.25;
    float Q = 43e6;
    float eta = 0.3;
    float P = domPtr->P.get_value(ijkCurr);
    float T_amb = thermo::theta_to_T(domPtr->T_POT.get_value(ijkCurr), P);
    float e_amb = thermo::r_to_e(domPtr->QV.get_value(ijkCurr), P);

    float G = thermo::calc_G(EI_H2O, P, Q, eta);

    float N_init = thermo::calc_N_initial(Q, eta, T_exhaust, T_amb);

    float e_exhaust = thermo::calc_e_exhaust(e_amb, P, EI_H2O, N_init);

    float T_LM = thermo::calc_T_LM(G);

    bool contrailForms = thermo::formation_condition_met(T_exhaust, e_exhaust, T_LM, G);
    int rc;
    std::string msg;
    msg = "Formation threshold reached: " + std::to_string(contrailForms);
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);

    if (contrailForms) {
        cross_section_area = 50;
        r_ice = 1e-6;
        n_ice = 1e12;
        N_ice_per_m = n_ice * cross_section_area;
        M_ice_per_m = r_to_v(r_ice) * ICE_DENSITY * N_ice_per_m;
    }
    else {
        isDead = true;
    }
}