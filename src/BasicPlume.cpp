#include <ESMC.h>
#include <cmath>
#include <iostream>
#include "plumeModels.h"
#include "domain.h"
#include "timekeeping.h"
#include "thermo.h"
#include "mathUtils.h"

using namespace mathUtils;

// Constants
const double ICE_DENSITY = 917; // Approx assumption (kg m-3)
const double H2O_VOL_ICE = thermo::H2O_MOLECULAR_MASS / ICE_DENSITY; // (m3)

// Parameters
const float MIN_M_ICE_PER_M = 1e-4; // kg m-1

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
    float S_i = e_amb/thermo::Buck_ice(T_amb);

    float air_diffusivity = 2.11e-5 * std::pow(T_amb/273.15, 1.94) * (101325 / P_amb);
    float vapour_thermal_speed = std::sqrt(8 * thermo::BOLTZMANN_CONSTANT * T_amb / (PI*thermo::H2O_MOLECULAR_MASS));
    // Number concentration of water vapour
    float n_vap = thermo::AVOGADRO_CONSTANT * e_amb / (thermo::IDEAL_GAS_CONSTANT * T_amb);

    float correction_factor = 1 + accom_coeff * vapour_thermal_speed * r_ice / (4 * air_diffusivity);
    // Flux of water vapour molecules to a crystal (s-1)
    float J = (PI * r_ice * r_ice * accom_coeff * vapour_thermal_speed * n_vap) / correction_factor * (S_i - 1);

    // Growth rate of a crystal (m3 s-1)
    float dv_dt = H2O_VOL_ICE * J;
    // Change in volume of a crystal (m3) capped at crystal size
    float dv = std::max(dv_dt * duration_s, -r_to_v(r_ice));

    // Update crystal size
    r_ice = v_to_r(r_to_v(r_ice) + dv);

    // Update mass per metre
    M_ice_per_m = r_to_v(r_ice) * ICE_DENSITY * N_ice_per_m;

    if (domPtr->onlineCoupling) {
        // Update ambient water vapour mixing ratio
        float vapour_mass_uptake_per_crystal = (dv/H2O_VOL_ICE) * thermo::H2O_MOLECULAR_MASS;
        float vapour_mass_uptake_tot = vapour_mass_uptake_per_crystal * N_ice_per_m * length;
        float grid_dry_mass = domPtr->DRYMASS.get_value(ijkCurr);
        *domPtr->QV.get(ijkCurr) -= vapour_mass_uptake_tot/grid_dry_mass;

        int rc;
        std::string msg;
        msg = "Vapour uptake per crystal (kg) = " + std::to_string(vapour_mass_uptake_per_crystal);
        rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
        msg = "Total vapour uptake (kg) = " + std::to_string(vapour_mass_uptake_tot);
        rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
        msg = "Delta QV (kg kg-1) = " + std::to_string(-vapour_mass_uptake_tot/grid_dry_mass * 1e6) + " * 1e-6";
        rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
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
        cross_section_area = 300;
        r_ice = 1e-6;
        n_ice = 1e9;
        N_ice_per_m = n_ice * cross_section_area;
        M_ice_per_m = r_to_v(r_ice) * ICE_DENSITY * N_ice_per_m;
    }
    else {
        isDead = true;
    }
}