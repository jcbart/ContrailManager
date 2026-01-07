#include <ESMC.h>
#include "plumeModels.h"
#include "domain.h"
#include "timekeeping.h"
#include "thermo.h"

void SegmentBasicPlume::integrate(const CMTime& timeStepStart, const CMTime& timeStepEnd) {
    if (!doneFormation) {
        formation();
        doneFormation = true;
    }
}

void SegmentBasicPlume::dump() {
    // Only really need to do this if formation happened
    IDX3 ijkCurr;
    // Should be safe to ignore return value
    bool inGrid = domPtr->loc_to_ijk(centre, ijkCurr);
    float grid_dry_mass = domPtr->DRYMASS.get_value(ijkCurr);
    *domPtr->deltaQI.get(ijkCurr) += m_ice/grid_dry_mass;
}

void SegmentBasicPlume::addToQIcontrail() {
    IDX3 ijkCurr;
    // Should be safe to ignore return value
    bool inGrid = domPtr->loc_to_ijk(centre, ijkCurr);
    float grid_dry_mass = domPtr->DRYMASS.get_value(ijkCurr);
    *domPtr->QIcontrail.get(ijkCurr) += m_ice/grid_dry_mass;
    int rc;
    std::string msg;
    msg = "QIcontrail at " + ijkCurr.asString() + " set to " + std::to_string(*domPtr->QIcontrail.get(ijkCurr));
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
}

void SegmentBasicPlume::scaleWidthAfterAdvection(float lengthRatio) {
    // Has no width so do nothing
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
        m_ice = 1000;
    }
    else {
        isDead = true;
    }
}