#ifndef SEGMENTCOCIP_H
#define SEGMENTCOCIP_H

#ifdef WITH_COCIP

#include <CoCiP++/CoCiP.h>
#include <CoCiP++/met.h>
#include "segment/Segment.h"

// Forward declaration
struct Params;

// CoCiP segment type
struct SegmentCoCiP : public Segment {
    CoCiP<ArrayMet<float>> cocip;

    // Empty constructor
    SegmentCoCiP() : Segment() {}

    // Constructor
    SegmentCoCiP(const FlightInputs& flightInputs, Domain* domain,
        std::shared_ptr<Params> params) : Segment(flightInputs, domain) {
    
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
    
    double totalIceMass() const override {
        // Do q_to_r as long as plume_mass_per_m is actually dry air mass
        return thermo::q_to_r(cocip.iwc) * cocip.plume_mass_per_m * length;
    }

    double totalIceNumber() const override {
        return cocip.n_ice_per_m * length;
    }

    double effectiveRadius() const override {
        // Ratio between the volume mean radius and the effective radius used in CoCiP
        constexpr double c_r = 0.9;
        return cocip.r_ice_vol / c_r;
    }

    void formation() override;

    void evolve(const CMTime& timeStepStart, const CMTime& timeStepEnd) override;

    void dump() override;

    void addToQIcontrail() override;

    void addToNIcontrail() override;

private:
    // Update CoCiP's internal local meteorology; return false if interpolation requirement
    // is not satisfied (i.e. too low altitude)
    bool updateMet();
};

#endif

#endif