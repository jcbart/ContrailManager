#ifndef SERIALIZECOCIP_H
#define SERIALIZECOCIP_H

#ifdef WITH_COCIP

#include <cereal/types/base_class.hpp>
#include <cereal/types/chrono.hpp>
#include <CoCiP++/CoCiP.h>
#include <CoCiP++/met.h>
#include <CoCiP++/params.h>
#include <CoCiP++/CoCiPTime.h>
#include "segments/SegmentCoCiP.h"
#include "serialization/SerializeSegment.h"

namespace cereal {

// Save IMet
template<class Archive>
void save(Archive& ar, const IMet& met) {
    ar(
        met.dz_m,

        met.tnsr,
        met.olr,

        met.air_pressure,
        met.air_temperature,
        met.specific_humidity,
        met.u_wind,
        met.v_wind,

        met.air_pressure_old,
        met.air_temperature_old,
        met.specific_humidity_old,

        met.air_pressure_lower,
        met.air_temperature_lower,
        met.u_wind_lower,
        met.v_wind_lower,

        met.effective_vertical_resolution,

        met.dtheta_dz,
        met.ds_dz,
        met.dsn_dz,
        met.rho_air,
        met.rh_i,
        met.tau_cirrus,
        met.sdr,
        met.rsr
    );
}

// Load IMet
template<class Archive>
void load(Archive& ar, IMet& met) {
    double dz_m, tnsr, olr, air_pressure, air_temperature, specific_humidity, u_wind, v_wind,
        air_pressure_old, air_temperature_old, specific_humidity_old, air_pressure_lower,
        air_temperature_lower, u_wind_lower, v_wind_lower, effective_vertical_resolution,
        dtheta_dz, ds_dz, dsn_dz, rho_air, rh_i, tau_cirrus, sdr, rsr;
    
    ar(dz_m, tnsr, olr, air_pressure, air_temperature, specific_humidity, u_wind, v_wind,
        air_pressure_old, air_temperature_old, specific_humidity_old, air_pressure_lower,
        air_temperature_lower, u_wind_lower, v_wind_lower, effective_vertical_resolution,
        dtheta_dz, ds_dz, dsn_dz, rho_air, rh_i, tau_cirrus, sdr, rsr);
    
    const_cast<double&>(met.dz_m) = dz_m;
    met.tnsr = tnsr;
    met.olr = olr;

    met.air_pressure = air_pressure;
    met.air_temperature = air_temperature;
    met.specific_humidity = specific_humidity;
    met.u_wind = u_wind;
    met.v_wind = v_wind;

    met.air_pressure_old = air_pressure_old;
    met.air_temperature_old = air_temperature_old;
    met.specific_humidity_old = specific_humidity_old;

    met.air_pressure_lower = air_pressure_lower;
    met.air_temperature_lower = air_temperature_lower;
    met.u_wind_lower = u_wind_lower;
    met.v_wind_lower = v_wind_lower;

    met.effective_vertical_resolution = effective_vertical_resolution;

    met.dtheta_dz = dtheta_dz;
    met.ds_dz = ds_dz;
    met.dsn_dz = dsn_dz;
    met.rho_air = rho_air;
    met.rh_i = rh_i;
    met.tau_cirrus = tau_cirrus;
    met.sdr = sdr;
    met.rsr = rsr;
}

template <class Archive>
struct specialize<Archive, IMet, cereal::specialization::non_member_load_save> {};

// Save ArrayMet<arrayType>
template<class Archive, typename arrayType>
void save(Archive& ar, const ArrayMet<arrayType>& met) {
    // Serialize dz_m and vsize for use in constructor, then rest of IMet class
    ar(
        met.vsize,
        cereal::virtual_base_class<IMet>(&met)
        // Other members are raw pointers which will be reset on next call to evolve
    );
}

// Custom loader for ArrayMet<arrayType> to handle const-member initialisation
template <typename arrayType>
struct LoadAndConstruct<ArrayMet<arrayType>> {
    template <class Archive>
    static void load_and_construct(Archive & ar, cereal::construct<ArrayMet<arrayType>> & construct) {
        int vsize;

        ar(vsize);

        // Use garbage dz_m value in constructor; set in IMet load
        construct(0, vsize);
        // Other members are raw pointers which will be reset on next call to evolve

        ar(cereal::virtual_base_class<IMet>(construct.ptr()));
    }
};

template <class Archive, typename arrayType>
struct specialize<Archive, ArrayMet<arrayType>, cereal::specialization::non_member_load_save> {};

// Serialize Params
template<class Archive>
void serialize(Archive& ar, Params& params) {
    ar(
        params.dz_m,
        params.initial_wake_vortex_depth,
        params.sedimentation_impact_factor,
        params.wind_shear_enhancement_exponent,
        params.min_ice_particle_number_nvpm_ei_n,
        params.max_depth,
        params.max_horizontal_diffusivity,
        params.max_vertical_diffusivity,
        params.radiative_heating_effects,
        params.radius_threshold_um,
        params.habit_distributions,
        params.rf_sw_enhancement_factor,
        params.rf_lw_enhancement_factor,
        params.min_tau,
        params.max_tau,
        params.min_n_ice_per_m3,
        params.max_n_ice_per_m3
    );
}

// Serialize CoCiPTime
template<class Archive>
void serialize(Archive& ar, CoCiPTime& datetime) {
    ar(
        datetime.timepoint
    );
}

// Serialize CoCiP
template<class Archive, typename arrayType>
void serialize(Archive& ar, CoCiP<ArrayMet<arrayType>>& cocip) {
    ar(
        cocip.met,
        cocip.params,

        cocip.longitude,
        cocip.latitude,
        cocip.altitude,
        cocip.datetime,

        cocip.sin_a,
        cocip.cos_a,

        cocip.engine_efficiency,
        cocip.ei_h2o,
        cocip.q_fuel,
        cocip.aircraft_mass,
        cocip.wingspan,
        cocip.true_airspeed,
        cocip.fuel_flow,
        cocip.nvpm_ei_n,

        cocip.sac,
        cocip.T_crit_sac,

        cocip.width,
        cocip.depth,
        cocip.area_eff,
        cocip.sigma_yz,
        cocip.n_ice_per_m,
        cocip.n_ice_per_vol,
        cocip.iwc,
        cocip.plume_mass_per_m,
        cocip.r_ice_vol,
        cocip.tau_contrail,
        cocip.terminal_fall_speed,
        cocip.diffuse_h,
        cocip.diffuse_v,
        cocip.dn_dt_agg,
        cocip.dn_dt_turb,
        cocip.heat_rate,
        cocip.d_heat_rate,
        cocip.rf_sw,
        cocip.rf_lw,
        cocip.rf_net,
        cocip.cumul_heat,
        cocip.cumul_differential_heat,
        cocip.persistent
    );
}

// Serialize SegmentCoCiP
template<class Archive>
void serialize(Archive& ar, SegmentCoCiP& seg) {
    // Serialize base class, then derived members
    ar(
        cereal::virtual_base_class<Segment>(&seg),
        seg.cocip,
        seg.doneFormation
    );
}

}

#endif

#endif