#ifdef WITH_COCIP

#include <omp.h>
#include <CoCiP++/CoCiP.h>
#include <CoCiP++/met.h>
#include <CoCiP++/params.h>
#include <CoCiP++/CoCiPTime.h>
#include "segments/SegmentCoCiP.h"
#include "thermo.h"
#include "map/types.h"
#include "CMLog.h"

SegmentCoCiP::SegmentCoCiP(const FlightInputs& flightInputs, std::shared_ptr<Domain> domain,
    std::shared_ptr<Params> params)
    : Segment(flightInputs, domain) {
    
    cocip.met = std::make_unique<ArrayMet<float>>(params->dz_m, domain->get_kSize());
    cocip.params = params;
    // give flight inputs (flightEmissions is taken from flightInputs)
    cocip.engine_efficiency = flightEmissions.engine_efficiency;
    cocip.ei_h2o = flightEmissions.ei_h2o;
    cocip.q_fuel = flightEmissions.q_fuel;
    cocip.aircraft_mass = flightEmissions.aircraft_mass;
    cocip.wingspan = flightEmissions.wingspan;
    cocip.true_airspeed = flightEmissions.true_airspeed;
    cocip.fuel_flow = flightEmissions.fuel_flow;
    cocip.nvpm_ei_n = flightEmissions.nvpm_ei_n;
}

bool SegmentCoCiP::updateMet() {
    IDX<3, int> ijk = domain->loc_to_ijk(centre);

    // Indices at surface of column containing contrail
    IDX<3, int> ijkSurface = ijk;
    ijkSurface[2] = domain->get_kds();

    // CoCiP requires that at least one grid cell centre be below the interpolation point
    // (at alt - dz_m) and at least one grid cell centre be above current altitude,
    // so return false if not
    if (centre.alt - cocip.params->dz_m < domain->Z.get(ijkSurface)
        || ijk[2] >= domain->get_kde()) {
        return false;
    }

    cocip.met->T_POT = domain->T_POT.get_ptr(ijkSurface);
    cocip.met->P = domain->P.get_ptr(ijkSurface);
    cocip.met->QV = domain->QV.get_ptr(ijkSurface);
    cocip.met->U = domain->U.get_ptr(ijkSurface);
    cocip.met->V = domain->V.get_ptr(ijkSurface);
    cocip.met->CIWC = domain->QI.get_ptr(ijkSurface);
    cocip.met->Z = domain->Z.get_ptr(ijkSurface);
    cocip.met->Z_AT_W = domain->Z_AT_W.get_ptr(ijkSurface);

    cocip.met->tnsr = domain->TNSR.get(ijk);
    cocip.met->olr = domain->OLR.get(ijk);
    return true;
}

void SegmentCoCiP::evolve(const CMTime& timeStepStart, const CMTime& timeStepEnd) {
    // Give CoCiP current location and datetime
    cocip.longitude = centre.lon;
    cocip.latitude = centre.lat;
    cocip.altitude = centre.alt;
    cocip.datetime.set(timeStepEnd.timepoint);

    // Get local meteorology
    if (!updateMet()) {
        outOfBounds = true;
        return;
    }
    
    if (!doneFormation) {
        cocip.formation();

        double fuel_dist = cocip.fuel_flow / cocip.true_airspeed; // (kg fuel m-1)
        double vap_mass_per_m_exhaust = cocip.ei_h2o * fuel_dist; // (kg vapour m-1)
        // Mass of vapour exhausted (kg) - used in both formation and no formation
        double M_v_exhaust = vap_mass_per_m_exhaust * length;

        if (!cocip.sac) {
            isDead = true;
            //CM_LogWrite("CoCiP: no formation");

            // If two-way coupling, return water vapour to atmosphere
            if (domain->twoWayCoupling) {
                // Get dry mass of grid cell contrail is inside
                IDX<3, int> ijk = domain->loc_to_ijk(centre);

                double gridDryMass = domain->DRYMASS.get(ijk);

                // Add M_v_exhaust to current grid cell
                domain->deltaQV.add(ijk,  M_v_exhaust / gridDryMass);
            }
            return;
        }

        cocip.simulate_wake_vortex_downwash();
        cocip.initial_properties();

        if (!cocip.persistent) {
            isDead = true;
            //CM_LogWrite("CoCiP: not initially persistent");

            // If two-way coupling, return water vapour to atmosphere and assume !cocip.persistent
            // is because IWC -> 0
            if (domain->twoWayCoupling) {
                // Get dry mass of grid cell contrail is inside
                IDX<3, int> ijk = domain->loc_to_ijk(centre);

                double gridDryMass = domain->DRYMASS.get(ijk);

                // Add M_v_exhaust to current grid cell
                domain->deltaQV.add(ijk,  M_v_exhaust / gridDryMass);
            }
            return;
        }

        // Takes angle between segment and longitude axis
        cocip.process_downwash_flight(90 - heading);

        double M_ice = totalIceMass();

        // Specific humidity inside contrail
        double q_sat = thermo::q_sat_ice(cocip.met->air_temperature, cocip.met->air_pressure);
        // Do q_to_r as long as plume_mass_per_m is actually dry air mass
        double M_v_inside = thermo::q_to_r(q_sat) * cocip.plume_mass_per_m * length;

        // Vapour mass intaken from atmosphere =
        //    ice mass + vapour mass inside - vapour mass exhausted
        M_v_accum = M_ice + M_v_inside - M_v_exhaust;

        doneFormation = true;
    }

    // Plume mass before time step (kg)
    double M_air_before = cocip.plume_mass_per_m * length;

    double dt_s = (birthTime > timeStepStart)
        ? (timeStepEnd - birthTime).to_s()
        : (timeStepEnd - timeStepStart).to_s();
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

    // The below is only required for a two-way coupling in which some water vapour is continually
    // exchanged through sedimentation
    if (domain->twoWayCoupling) {
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
        IDX<3, int> ijk = domain->loc_to_ijk(centre);

        double gridDryMass = domain->DRYMASS.get(ijk);

        // Remove M_v_sed from current grid cell
        domain->deltaQV.subtract(ijk,  M_v_sed / gridDryMass);
    }
}

void SegmentCoCiP::dump() {
    IDX<3, int> ijk = domain->loc_to_ijk(centre);

    double gridDryMass = domain->DRYMASS.get(ijk);

    // Specific humidity inside contrail
    double q_sat = thermo::q_sat_ice(cocip.met->air_temperature, cocip.met->air_pressure);
    // Water vapour mass (returned is mass inside minus that double-counted from atmosphere)
    // Do q_to_r as long as plume_mass_per_m is actually dry air mass
    double M_v = thermo::q_to_r(q_sat) * cocip.plume_mass_per_m * length;
    domain->deltaQV.add(ijk, (M_v - M_v_accum) / gridDryMass);

    // Ice mass
    double M_ice = totalIceMass();
    domain->deltaQI.add(ijk, M_ice / gridDryMass);

    // Ice number
    double N_ice = cocip.n_ice_per_m * length;
    domain->deltaNI.add(ijk, N_ice / gridDryMass);
}

void SegmentCoCiP::addToQIcontrail() {
    IDX<3, int> ijk = domain->loc_to_ijk(centre);

    double M_ice = totalIceMass();
    float gridDryMass = domain->DRYMASS.get(ijk);
    domain->QIcontrail.add(ijk, M_ice / gridDryMass);
}

#endif