#ifdef WITH_COCIP

#include <CoCiP++/CoCiP.h>
#include <CoCiP++/met.h>
#include <CoCiP++/params.h>
#include <CoCiP++/CoCiPTime.h>
#include "Segment.h"
#include "SegmentCoCiP.h"
#include "thermo.h"
#include "mapUtils.h"
#include "CMLog.h"

SegmentCoCiP::SegmentCoCiP(const std::string& parentID, const CMTime& birthTime,
    const FlightInputs& flightInputs, IDomain* domPtr, const Geo3D& backLoc, const Geo3D& frontLoc,
    const float length, std::shared_ptr<Params> params)
    : Segment(parentID, birthTime, flightInputs, domPtr, backLoc, frontLoc, length) {
    
    cocip.met = std::unique_ptr<ArrayMet<float>>(new ArrayMet<float>(params->dz_m, domPtr->get_altSize()));
    cocip.params = params;
    // give flight inputs
    cocip.engine_efficiency = flightInputs.engine_efficiency;
    cocip.ei_h2o = flightInputs.ei_h2o;
    cocip.q_fuel = flightInputs.q_fuel;
    cocip.aircraft_mass = flightInputs.aircraft_mass;
    cocip.wingspan = flightInputs.wingspan;
    cocip.true_airspeed = flightInputs.true_airspeed;
    cocip.fuel_flow = flightInputs.fuel_flow;
    cocip.T_exhaust = flightInputs.T_exhaust;
    cocip.nvpm_ei_n = flightInputs.nvpm_ei_n;
}

void SegmentCoCiP::updateMet() {
    IDX3<int> ijkCurr;
    bool inGrid = domPtr->loc_to_ijk(centre, ijkCurr);
    if (!inGrid) { CM_RaiseUnexpectedOutOfBounds(centre, __FILE__, __LINE__); }

    // Indices at surface of column containing contrail
    IDX3<int> ijkSurface = ijkCurr;
    ijkSurface.k = domPtr->get_kds();

    cocip.met->T_POT = domPtr->T_POT.get(ijkSurface);
    cocip.met->P = domPtr->P.get(ijkSurface);
    cocip.met->QV = domPtr->QV.get(ijkSurface);
    cocip.met->U = domPtr->U.get(ijkSurface);
    cocip.met->V = domPtr->V.get(ijkSurface);
    cocip.met->CIWC = domPtr->QI.get(ijkSurface);
    cocip.met->Z = domPtr->Z.get(ijkSurface);
    cocip.met->Z_AT_W = domPtr->Z_AT_W.get(ijkSurface);

    cocip.met->tnsr = domPtr->TNSR.get_value(ijkCurr);
    cocip.met->olr = domPtr->OLR.get_value(ijkCurr);
}

void SegmentCoCiP::integrate(const CMTime& timeStepStart, const CMTime& timeStepEnd) {
    // Give CoCiP current location and datetime
    cocip.longitude = centre.lon;
    cocip.latitude = centre.lat;
    cocip.altitude = centre.alt;
    cocip.datetime.set(
        timeStepEnd.yy,
        timeStepEnd.mm,
        timeStepEnd.dd,
        timeStepEnd.h,
        timeStepEnd.m,
        timeStepEnd.s
    );

    // Get local meteorology
    updateMet();
    
    if (!doneFormation) {
        cocip.formation();

        double fuel_dist = cocip.fuel_flow / cocip.true_airspeed; // (kg fuel m-1)
        double vap_mass_per_m_exhaust = cocip.ei_h2o * fuel_dist; // (kg vapour m-1)
        // Mass of vapour exhausted (kg) - used in both formation and no formation
        double M_v_exhaust = vap_mass_per_m_exhaust * length;

        if (!cocip.sac) {
            isDead = true;
            CM_LogWrite("CoCiP: no formation");

            // If two-way coupling, return water vapour to atmosphere
            if (domPtr->twoWayCoupling) {
                // Get dry mass of grid cell contrail is inside
                IDX3<int> ijkCurr;
                bool inGrid = domPtr->loc_to_ijk(centre, ijkCurr);
                if (!inGrid) { CM_RaiseUnexpectedOutOfBounds(centre, __FILE__, __LINE__); }

                double gridDryMass = domPtr->DRYMASS.get_value(ijkCurr);

                // Add M_v_exhaust to current grid cell
                *domPtr->deltaQV.get(ijkCurr) += M_v_exhaust / gridDryMass;
            }
            return;
        }

        cocip.simulate_wake_vortex_downwash();
        cocip.initial_properties();

        if (!cocip.persistent) {
            isDead = true;
            CM_LogWrite("CoCiP: not initially persistent");

            // If two-way coupling, return water vapour to atmosphere and assume !cocip.persistent
            // is because IWC -> 0
            if (domPtr->twoWayCoupling) {
                // Get dry mass of grid cell contrail is inside
                IDX3<int> ijkCurr;
                bool inGrid = domPtr->loc_to_ijk(centre, ijkCurr);
                if (!inGrid) { CM_RaiseUnexpectedOutOfBounds(centre, __FILE__, __LINE__); }

                double gridDryMass = domPtr->DRYMASS.get_value(ijkCurr);

                // Add M_v_exhaust to current grid cell
                *domPtr->deltaQV.get(ijkCurr) += M_v_exhaust / gridDryMass;
            }
            return;
        }

        // Takes angle between segment and longitude axis
        cocip.process_downwash_flight(90 - heading);

        // Do q_to_r as long as plume_mass_per_m is actually dry air mass
        double M_ice = thermo::q_to_r(cocip.iwc) * cocip.plume_mass_per_m * length;

        // Specific humidity inside contrail
        double q_sat = thermo::q_sat_ice(cocip.met->air_temperature, cocip.met->air_pressure);
        // Do q_to_r as long as plume_mass_per_m is actually dry air mass
        double M_v_inside = thermo::q_to_r(q_sat) * cocip.plume_mass_per_m * length;

        // Vapour mass intaken from atmosphere =
        //    ice mass + vapour mass inside - vapour mass exhausted
        M_v_accum = M_ice + M_v_inside - M_v_exhaust;

        CM_LogWrite("CoCiP M_v_exhaust: " + std::to_string(M_v_exhaust)
            + ", M_ice (initial): " + std::to_string(M_ice)
            + ", M_v_accum (initial): " + std::to_string(M_v_accum));

        doneFormation = true;
    }

    // Plume mass before time step (kg)
    double M_air_before = cocip.plume_mass_per_m * length;

    double dt_s = (birthTime > timeStepStart)
        ? (timeStepEnd - birthTime).dhms_to_s()
        : (timeStepEnd - timeStepStart).dhms_to_s();
    // Takes angle between segment and longitude axis
    cocip.evolve(lengthRatio, 90 - heading, dt_s);
    
    isDead = !cocip.persistent;

    // Get change in altitude after CoCiP sediments
    double deltaAlt = cocip.altitude - centre.alt;
    centre.alt += deltaAlt;
    front.alt += deltaAlt;
    back.alt += deltaAlt;

    // Plume mass after time step (kg)
    double M_air_after = cocip.plume_mass_per_m * length;

    // Add to mass of accumulated ambient water vapour
    // using the interpolated humidity found by CoCiP
    M_v_accum += cocip.met->specific_humidity * (M_air_after - M_air_before);

    CM_LogWrite("CoCiP width: " + std::to_string(cocip.width) + ", depth: "
        + std::to_string(cocip.depth) + ", IWC: " + std::to_string(cocip.iwc) + ", M_v_accum: "
        + std::to_string(M_v_accum));

    // The below is only required for a two-way coupling in which some water vapour is continually
    // exchanged through sedimentation
    if (domPtr->twoWayCoupling) {
        // Calculate ambient conditions seen by CoCiP using its interpolation method
        int k_below = cocip.met->find_k_below(cocip.altitude);
        double interp_fraction = cocip.met->calc_interp_fraction(cocip.altitude, k_below);

        // Ambient air pressure (Pa)
        double P_amb = cocip.met->interp_P(k_below, interp_fraction);
        // Ambient water vapour mass mixing ratio (kg (kg dry air)-1)
        double r_v_amb = cocip.met->interp_QV(k_below, interp_fraction);
        // Ambient air temperature (kg m-3)
        double rho_d_amb = thermo::rho_d(
            thermo::theta_to_T(cocip.met->interp_T_POT(k_below, interp_fraction), P_amb),
            P_amb
        );
        // Contrail water vapour mass mixing ratio - CoCiP assumes saturation (kg (kg dry air)-1)
        double r_v_contrail = thermo::q_to_r(thermo::q_sat_ice(
            cocip.met->air_temperature,
            cocip.met->air_pressure
        ));
        // Contrail air temperature (kg m-3)
        double rho_d_contrail = cocip.met->rho_air;

        // Cross-sectional area sedimented through (m2) - if the segment sediments through a
        // greater cross-sectional area than its own, cap at its own
        double area_swept = std::min(std::abs(deltaAlt) * cocip.width, cocip.area_eff);
        
        // Mass of vapour gained by the segment through sedimentation (kg)
        double M_v_sed = (r_v_amb * rho_d_amb - r_v_contrail * rho_d_contrail)
                         * area_swept * length;
        
        // Get dry mass of grid cell contrail is inside
        IDX3<int> ijkCurr;
        bool inGrid = domPtr->loc_to_ijk(centre, ijkCurr);
        if (!inGrid) { CM_RaiseUnexpectedOutOfBounds(centre, __FILE__, __LINE__); }

        double gridDryMass = domPtr->DRYMASS.get_value(ijkCurr);

        // Remove M_v_sed from current grid cell
        *domPtr->deltaQV.get(ijkCurr) -= M_v_sed / gridDryMass;
    }
}

void SegmentCoCiP::dump() {
    IDX3<int> ijkCurr;
    bool inGrid = domPtr->loc_to_ijk(centre, ijkCurr);
    if (!inGrid) { CM_RaiseUnexpectedOutOfBounds(centre, __FILE__, __LINE__); }

    double gridDryMass = domPtr->DRYMASS.get_value(ijkCurr);

    // Specific humidity inside contrail
    double q_sat = thermo::q_sat_ice(cocip.met->air_temperature, cocip.met->air_pressure);
    // Water vapour mass (returned is mass inside minus that double-counted from atmosphere)
    // Do q_to_r as long as plume_mass_per_m is actually dry air mass
    double M_v = thermo::q_to_r(q_sat) * cocip.plume_mass_per_m * length;
    *domPtr->deltaQV.get(ijkCurr) += (M_v - M_v_accum) / gridDryMass;

    // Ice mass
    // Do q_to_r as long as plume_mass_per_m is actually dry air mass
    double M_ice = thermo::q_to_r(cocip.iwc) * cocip.plume_mass_per_m * length;
    *domPtr->deltaQI.get(ijkCurr) += M_ice / gridDryMass;

    // Ice number
    double N_ice = cocip.n_ice_per_m * length;
    *domPtr->deltaNI.get(ijkCurr) += N_ice / gridDryMass;
}

void SegmentCoCiP::addToQIcontrail() {
    IDX3<int> ijkCurr;
    bool inGrid = domPtr->loc_to_ijk(centre, ijkCurr);
    if (!inGrid) { CM_RaiseUnexpectedOutOfBounds(centre, __FILE__, __LINE__); }

    // Do q_to_r as long as plume_mass_per_m is actually dry air mass
    float M_ice = thermo::q_to_r(cocip.iwc) * cocip.plume_mass_per_m * length;
    float gridDryMass = domPtr->DRYMASS.get_value(ijkCurr);
    *domPtr->QIcontrail.get(ijkCurr) += M_ice / gridDryMass;
}

#endif