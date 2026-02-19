#ifdef WITH_COCIP

#include <ESMC.h>
#include <CoCiP++/CoCiP.h>
#include <CoCiP++/met.h>
#include <CoCiP++/params.h>
#include <CoCiP++/CoCiPTime.h>
#include "Segment.h"
#include "SegmentCoCiP.h"
#include "thermo.h"

SegmentCoCiP::SegmentCoCiP(const std::string& parentID, const CMTime& birthTime,
    const FlightInputs& flightInputs, Domain* domPtr, const Geo3D& backLoc, const Geo3D& frontLoc,
    const float length, Params* params)
    : Segment(parentID, birthTime, flightInputs, domPtr, backLoc, frontLoc, length) {
    
    cocip.met = new ArrayMet<float>(domPtr->get_altSize());
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
    IDX3 ijkCurr;
    // Should be safe to ignore return value
    bool inGrid = domPtr->loc_to_ijk(centre, ijkCurr);

    // Indices at surface of column containing contrail
    IDX3 ijkSurface = ijkCurr;
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
    // Get local meteorology
    updateMet();

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
    
    if (!doneFormation) {
        cocip.formation();

        if (!cocip.sac) {
            isDead = true;
            int rc = ESMC_LogWrite("CoCiP: no formation", ESMC_LOGMSG_INFO);
            return;
        }

        cocip.simulate_wake_vortex_downwash();
        cocip.initial_properties();

        if (!cocip.persistent) {
            isDead = true;
            int rc = ESMC_LogWrite("CoCiP: not initially persistent", ESMC_LOGMSG_INFO);
            return;
        }

        // Takes angle between segment and longitude axis
        cocip.process_downwash_flight(90 - heading);
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

    // Location after cocip.evolve
    IDX3 ijkCurr;
    // Should be safe to ignore return value
    bool inGrid = domPtr->loc_to_ijk(centre, ijkCurr);

    // Plume mass after time step (kg)
    double M_air_after = cocip.plume_mass_per_m * length;

    // Add to mass of accumulated ambient water vapour
    M_v_accum += domPtr->QV.get_value(ijkCurr) * (M_air_after - M_air_before);

    int rc;
    std::string msg;
    msg = "CoCiP width: " + std::to_string(cocip.width) + ", depth: " + std::to_string(cocip.depth) + ", IWC: " + std::to_string(cocip.iwc) + ", M_v_accum: " + std::to_string(M_v_accum);
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);

    // The below is only required for a two-way coupling in which some water vapour is continually
    // exchanged through sedimentation
    if (domPtr->twoWayCoupling) {

        double P_amb = domPtr->P.get_value(ijkCurr);

        double r_v_amb = domPtr->QV.get_value(ijkCurr);
        double rho_d_amb = thermo::rho_d(
            thermo::theta_to_T(domPtr->T_POT.get_value(ijkCurr), P_amb),
            P_amb
        );
        double r_v_contrail = thermo::q_to_r(thermo::q_sat_ice(
            cocip.met->air_temperature,
            cocip.met->air_pressure
        ));
        double rho_d_contrail = cocip.met->rho_air;
        
        // Mass of vapour gained by the segment through sedimentation (kg)
        // If the segment sediments beyond its depth, cap at depth
        double M_v_sed = (r_v_amb * rho_d_amb - r_v_contrail * rho_d_contrail)
                         * cocip.width * length * std::min(std::abs(deltaAlt), cocip.depth);
        
        double gridDryMass = domPtr->DRYMASS.get_value(ijkCurr);

        // Remove M_v_sed from current grid cell
        *domPtr->deltaQV.get(ijkCurr) -= M_v_sed / gridDryMass;
    }
}

void SegmentCoCiP::dump() {
    IDX3 ijkCurr;
    // Should be safe to ignore return value
    bool inGrid = domPtr->loc_to_ijk(centre, ijkCurr);

    double gridDryMass = domPtr->DRYMASS.get_value(ijkCurr);

    // Ice mass
    double M_ice = cocip.iwc * cocip.plume_mass_per_m * length;
    *domPtr->deltaQI.get(ijkCurr) += M_ice / gridDryMass;

    // Water vapour mass (returned is mass inside minus that double-counted from atmosphere)
    double M_v = cocip.met->specific_humidity * cocip.plume_mass_per_m * length;
    *domPtr->deltaQV.get(ijkCurr) += (M_v - M_v_accum) / gridDryMass;

    // Ice number
    double N_ice = cocip.n_ice_per_m * length;
    *domPtr->deltaNI.get(ijkCurr) += N_ice / gridDryMass;
}

void SegmentCoCiP::addToQIcontrail() {
    IDX3 ijkCurr;
    // Should be safe to ignore return value
    bool inGrid = domPtr->loc_to_ijk(centre, ijkCurr);

    float M_ice = cocip.iwc * cocip.plume_mass_per_m * length;
    float gridDryMass = domPtr->DRYMASS.get_value(ijkCurr);
    *domPtr->QIcontrail.get(ijkCurr) += M_ice / gridDryMass;
}

#endif