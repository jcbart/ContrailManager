#ifndef SERIALIZECOCIP_H
#define SERIALIZECOCIP_H

#ifdef WITH_COCIP

#include <cereal/types/base_class.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/chrono.hpp>
#include <CoCiP++/CoCiP.h>
#include <CoCiP++/met.h>
#include <CoCiP++/params.h>
#include <CoCiP++/CoCiPTime.h>
#include "SegmentCoCiP.h"
#include "SerializeSegment.h"

namespace cereal {

// Serialize IMet
template<class Archive>
void serialize(Archive& ar, IMet& met) {
    ar(
        CEREAL_NVP(met.dz_m),

        CEREAL_NVP(met.tnsr),
        CEREAL_NVP(met.olr),

        CEREAL_NVP(met.air_pressure),
        CEREAL_NVP(met.air_temperature),
        CEREAL_NVP(met.specific_humidity),
        CEREAL_NVP(met.u_wind),
        CEREAL_NVP(met.v_wind),

        CEREAL_NVP(met.air_pressure_old),
        CEREAL_NVP(met.air_temperature_old),
        CEREAL_NVP(met.specific_humidity_old),

        CEREAL_NVP(met.air_pressure_lower),
        CEREAL_NVP(met.air_temperature_lower),
        CEREAL_NVP(met.u_wind_lower),
        CEREAL_NVP(met.v_wind_lower),

        CEREAL_NVP(met.effective_vertical_resolution),

        CEREAL_NVP(met.dtheta_dz),
        CEREAL_NVP(met.ds_dz),
        CEREAL_NVP(met.dsn_dz),
        CEREAL_NVP(met.rho_air),
        CEREAL_NVP(met.rh_i),
        CEREAL_NVP(met.tau_cirrus),
        CEREAL_NVP(met.sdr),
        CEREAL_NVP(met.rsr)
    );
}

// Serialize ArrayMet<arrayType>
template<class Archive, typename arrayType>
void serialize(Archive& ar, ArrayMet<arrayType>& met) {
    // Serialize dz_m and vsize for use in constructor, then rest of IMet class
    ar(
        CEREAL_NVP(met.vsize),
        cereal::base_class<IMet>(&met)
        // Other members are raw pointers which will be reset on next call to evolve
    );
}

// Custom loader for ArrayMet<arrayType> to handle const-member initialisation
template <typename arrayType>
struct LoadAndConstruct<ArrayMet<arrayType>> {
    template <class Archive>
    static void load_and_construct(Archive & ar, cereal::construct<ArrayMet<arrayType>> & construct) {
        int vsize;
        double dz_m;

        ar(vsize, dz_m); 

        construct(dz_m, vsize);
    }
};

// Serialize Params
template<class Archive>
void serialize(Archive& ar, Params& params) {
    ar(
        CEREAL_NVP(params.dz_m),
        CEREAL_NVP(params.initial_wake_vortex_depth),
        CEREAL_NVP(params.sedimentation_impact_factor),
        CEREAL_NVP(params.wind_shear_enhancement_exponent),
        CEREAL_NVP(params.min_ice_particle_number_nvpm_ei_n),
        CEREAL_NVP(params.max_depth),
        CEREAL_NVP(params.max_horizontal_diffusivity),
        CEREAL_NVP(params.max_vertical_diffusivity),
        CEREAL_NVP(params.radiative_heating_effects),
        CEREAL_NVP(params.radius_threshold_um),
        CEREAL_NVP(params.habit_distributions),
        CEREAL_NVP(params.rf_sw_enhancement_factor),
        CEREAL_NVP(params.rf_lw_enhancement_factor),
        CEREAL_NVP(params.min_tau),
        CEREAL_NVP(params.max_tau),
        CEREAL_NVP(params.min_n_ice_per_m3),
        CEREAL_NVP(params.max_n_ice_per_m3)
    );
}

// Serialize CoCiPTime
template<class Archive>
void serialize(Archive& ar, CoCiPTime& datetime) {
    ar(
        CEREAL_NVP(datetime.timepoint)
    );
}

// Serialize CoCiP
template<class Archive, typename arrayType>
void serialize(Archive& ar, CoCiP<ArrayMet<arrayType>>& cocip) {
    ar(
        CEREAL_NVP(cocip.met),
        CEREAL_NVP(cocip.params),

        CEREAL_NVP(cocip.longitude),
        CEREAL_NVP(cocip.latitude),
        CEREAL_NVP(cocip.altitude),
        CEREAL_NVP(cocip.datetime),

        CEREAL_NVP(cocip.sin_a),
        CEREAL_NVP(cocip.cos_a),

        CEREAL_NVP(cocip.engine_efficiency),
        CEREAL_NVP(cocip.ei_h2o),
        CEREAL_NVP(cocip.q_fuel),
        CEREAL_NVP(cocip.aircraft_mass),
        CEREAL_NVP(cocip.wingspan),
        CEREAL_NVP(cocip.true_airspeed),
        CEREAL_NVP(cocip.fuel_flow),
        CEREAL_NVP(cocip.nvpm_ei_n),

        CEREAL_NVP(cocip.sac),
        CEREAL_NVP(cocip.T_crit_sac),

        CEREAL_NVP(cocip.width),
        CEREAL_NVP(cocip.depth),
        CEREAL_NVP(cocip.area_eff),
        CEREAL_NVP(cocip.sigma_yz),
        CEREAL_NVP(cocip.n_ice_per_m),
        CEREAL_NVP(cocip.n_ice_per_vol),
        CEREAL_NVP(cocip.iwc),
        CEREAL_NVP(cocip.plume_mass_per_m),
        CEREAL_NVP(cocip.r_ice_vol),
        CEREAL_NVP(cocip.tau_contrail),
        CEREAL_NVP(cocip.terminal_fall_speed),
        CEREAL_NVP(cocip.diffuse_h),
        CEREAL_NVP(cocip.diffuse_v),
        CEREAL_NVP(cocip.dn_dt_agg),
        CEREAL_NVP(cocip.dn_dt_turb),
        CEREAL_NVP(cocip.heat_rate),
        CEREAL_NVP(cocip.d_heat_rate),
        CEREAL_NVP(cocip.rf_sw),
        CEREAL_NVP(cocip.rf_lw),
        CEREAL_NVP(cocip.rf_net),
        CEREAL_NVP(cocip.cumul_heat),
        CEREAL_NVP(cocip.cumul_differential_heat),
        CEREAL_NVP(cocip.persistent)
    );
}

// Serialize SegmentCoCiP
template<class Archive>
void serialize(Archive& ar, SegmentCoCiP& seg) {
    // Serialize base class, then derived members
    ar(
        cereal::base_class<Segment>(&seg),
        CEREAL_NVP(seg.cocip),
        CEREAL_NVP(seg.doneFormation)
    );
}

}

// Register ArrayMet<float> as a derived IMet
//CEREAL_REGISTER_TYPE(ArrayMet<float>)
//CEREAL_REGISTER_POLYMORPHIC_RELATION(IMet, ArrayMet<float>)

// Register SegmentCoCiP as a derived Segment
//CEREAL_REGISTER_TYPE(SegmentCoCiP)
//CEREAL_REGISTER_POLYMORPHIC_RELATION(Segment, SegmentCoCiP)

#endif

#endif