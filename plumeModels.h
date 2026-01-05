#ifndef PLUMEMODELS
#define PLUMEMODELS

#include <string>
#include "domain.h"
#include "segment.h"
#include "timekeeping.h"
#include "thermo.h"

// Plume model IDs

const int MODEL_ID_BASIC_PLUME = 1;

// Plume model names

const std::string MODEL_STR_BASIC_PLUME = "Basic plume model";


struct SegmentBasicPlume : public Segment {
    // Plume model-specific data
    bool doneFormation = false;
    float m_ice = 0; // kg

    // Plume model-specific integration method
    void integrate(const CMTime& timeStepStart, const CMTime& timeStepEnd) override {
        if (!doneFormation) {
            formation();
            doneFormation = true;
        }
    }

    // Plume model-specific dump method
    void dump() override {
        // Only really need to do this if formation happened
        IDX3 ijkCurr;
        // Should be safe to ignore return value
        bool inGrid = domPtr->loc_to_ijk(centre, ijkCurr);
        float grid_dry_mass = domPtr->DRYMASS.get_value(ijkCurr);
        *domPtr->deltaQI.get(ijkCurr) += m_ice/grid_dry_mass;
    }

    // Plume model-specific method to add contrail ice mass to QIcontrail field
    void addToQIcontrail() override {
        IDX3 ijkCurr;
        // Should be safe to ignore return value
        bool inGrid = domPtr->loc_to_ijk(centre, ijkCurr);
        float grid_dry_mass = domPtr->DRYMASS.get_value(ijkCurr);
        *domPtr->QIcontrail.get(ijkCurr) += m_ice/grid_dry_mass;
    }

    void formation() {
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

        if (contrailForms) {
            m_ice = 1000;
        }
        else {
            isDead = true;
        }
    }
};

#endif