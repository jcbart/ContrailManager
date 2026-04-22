#ifndef SEGMENTCACE_H
#define SEGMENTCACE_H

#include "Segment.h"

// Contrails as Cloud Enhancement (CaCE) segment type
struct SegmentCaCE : public Segment {
    // Model constants (default values from CoCiP)
    static constexpr float dz_m = 200;
    static constexpr float wind_shear_enhancement_exponent = 0.5;
    static constexpr float initial_wake_vortex_depth = 0.5;
    static constexpr float min_ice_particle_number_nvpm_ei_n = 1e13;

    // Flight emissions
    float engine_efficiency; // Engine efficiency ()
    float ei_h2o; // Emissions index of water vapour (kg (kg fuel)-1)
    float q_fuel; // Specific combustion heat of fuel (J kg-1)
    float aircraft_mass; // Aircraft mass (kg)
    float wingspan; // Aircraft wingspan (m)
    float true_airspeed; // True airspeed (m s-1)
    float fuel_flow; // Fuel flow (kg s-1)
    float nvpm_ei_n; // Emissions index of nvPM (# (kg fuel)-1)

    // Meteorological values
    float air_pressure = 0; // Pressure (Pa)
    float air_temperature = 0; // Air temperature (K)
    float specific_humidity = 0; // Specific humidity of water vapor (kg (kg moist air)-1)
    float u_wind = 0; // Eastward wind (m s-1)
    float v_wind = 0; // Northward wind (m s-1)
    float rho_air = 0; // Air density (kg m-3)
    float dtheta_dz = 0; // Potential temperature gradient (K m-1) (dT_dz in pycontrails)
    float ds_dz = 0; // Wind shear (m s-1 m-1)
    float dsn_dz = 0; // Wind shear normal (m s-1 m-1)
    float air_pressure_lower = 0; // Pressure (Pa) at altitude - dz_m (i.e. grid cell below)
    float air_temperature_lower = 0; // Temperature (K) at grid cell below
    float u_wind_lower = 0; // Eastward wind (m s-1) at grid cell below
    float v_wind_lower = 0; // Northward wind (m s-1) at grid cell below
    float effective_vertical_resolution = 0; // Effective vertical resolution of met data (m)

    float T_crit_sac = 0;

    double width = 0; // (m)
    double depth = 0; // (m)
    double area_eff = 0; // (m2)
    double iwc = 0; // Ice water content (kg (kg air)-1)
    double n_ice_per_m = 0; // Ice crystal number per unit length (# m-1)

    // Empty constructor
    SegmentCaCE() : Segment() {}

    // Constructor
    SegmentCaCE(const FlightInputs& flightInputs, std::shared_ptr<Domain> domain)
        : Segment(flightInputs, domain),
          engine_efficiency(flightEmissions.engine_efficiency),
          ei_h2o(flightEmissions.ei_h2o),
          q_fuel(flightEmissions.q_fuel),
          aircraft_mass(flightEmissions.aircraft_mass),
          wingspan(flightEmissions.wingspan),
          true_airspeed(flightEmissions.true_airspeed),
          fuel_flow(flightEmissions.fuel_flow),
          nvpm_ei_n(flightEmissions.nvpm_ei_n) {}

    double totalIceMass() const override {
        return iwc * rho_air * area_eff * length;
    }

    double effectiveRadius() const override {
        return 0; // N/A
    }

    void evolve(const CMTime& timeStepStart, const CMTime& timeStepEnd) override;

    void dump() override;

    void addToQIcontrail() override {
        return; // Should not have live contrail ice
    }

private:
    // Update local meteorological values
    void update_met();

    // Calculate the height fraction of altitude between Z[ijkBelow] and Z[ijkAbove]
    double calcInterpFraction(const double altitude,
        const IDX<3, int>& ijkBelow, const IDX<3, int>& ijkAbove) {
        
        return (altitude - domain->Z.get(ijkBelow))
            / (domain->Z.get(ijkAbove) - domain->Z.get(ijkBelow));
    }

    // Interpolate P array logarithmically given grid cell index below altitude (k_below) and
    // height fraction of altitude between grid cell centres (interpFraction)
    float interp_P(const IDX<3, int>& ijkBelow, const IDX<3, int>& ijkAbove,
        const double interpFraction) {
        
        return (
            std::pow(domain->P.get(ijkBelow), 1 - interpFraction)
            * std::pow(domain->P.get(ijkAbove), interpFraction)
        );
    }

    // Interpolate T_POT array linearly given grid cell index below altitude (k_below) and
    // height fraction of altitude between grid cell centres (interpFraction)
    float interp_T_POT(const IDX<3, int>& ijkBelow, const IDX<3, int>& ijkAbove,
        const double interpFraction) {

        return (domain->T_POT.get(ijkBelow)
            + interpFraction * (domain->T_POT.get(ijkAbove) - domain->T_POT.get(ijkBelow)));
    }

    // Interpolate QV array linearly given grid cell index below altitude (k_below) and
    // height fraction of altitude between grid cell centres (interpFraction)
    float interp_QV(const IDX<3, int>& ijkBelow, const IDX<3, int>& ijkAbove,
        const double interpFraction) {

        return (domain->QV.get(ijkBelow)
            + interpFraction * (domain->QV.get(ijkAbove) - domain->QV.get(ijkBelow)));
    }

    // Interpolate U array linearly given grid cell index below altitude (k_below) and
    // height fraction of altitude between grid cell centres (interpFraction)
    float interp_U(const IDX<3, int>& ijkBelow, const IDX<3, int>& ijkAbove,
        const double interpFraction) {
        
        return (domain->U.get(ijkBelow)
            + interpFraction * (domain->U.get(ijkAbove) - domain->U.get(ijkBelow)));
    }

    // Interpolate V array linearly given grid cell index below altitude (k_below) and
    // height fraction of altitude between grid cell centres (interpFraction)
    float interp_V(const IDX<3, int>& ijkBelow, const IDX<3, int>& ijkAbove,
        const double interpFraction) {
        
        return (domain->V.get(ijkBelow)
            + interpFraction * (domain->V.get(ijkAbove) - domain->V.get(ijkBelow)));
    }

    // Calculate formation; returns true if contrail forms according to SAC
    bool formation();

    // Simulate wake vortex downwash
    void simulate_wake_vortex_downwash();

    // Find initial properties
    void initial_properties();
};

#endif