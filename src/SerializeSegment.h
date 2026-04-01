#ifndef SERIALIZESEGMENT_H
#define SERIALIZESEGMENT_H

// Any archives must be included before types are registered
#include <cereal/archives/binary.hpp>
#include <cereal/types/memory.hpp>
#include "Segment.h"
#include "timekeeping.h"
#include "mapTypes.h"
#include "FlightInputs.h"

namespace cereal {

template<class Archive>
void serialize(Archive& ar, CMTime& datetime) {
    ar(
        CEREAL_NVP(datetime.timepoint)
    );
}

template<class Archive>
void serialize(Archive& ar, Geo3D& loc) {
    ar(
        CEREAL_NVP(loc.lon),
        CEREAL_NVP(loc.lat),
        CEREAL_NVP(loc.alt)
    );
}

template<class Archive>
void serialize(Archive& ar, FlightEmissions& emissions) {
    ar(
        CEREAL_NVP(emissions.engine_efficiency),
        CEREAL_NVP(emissions.ei_h2o),
        CEREAL_NVP(emissions.q_fuel),
        CEREAL_NVP(emissions.aircraft_mass),
        CEREAL_NVP(emissions.wingspan),
        CEREAL_NVP(emissions.true_airspeed),
        CEREAL_NVP(emissions.fuel_flow),
        CEREAL_NVP(emissions.nvpm_ei_n)
    );
}

template<class Archive>
void serialize(Archive& ar, Segment& seg) {
    ar(
        CEREAL_NVP(seg.ID),
        CEREAL_NVP(seg.parentID),
        CEREAL_NVP(seg.birthTime),
        CEREAL_NVP(seg.flightEmissions),
        // domain is not serialized; assigned upon restart

        CEREAL_NVP(seg.back),
        CEREAL_NVP(seg.front),
        CEREAL_NVP(seg.centre),
        CEREAL_NVP(seg.heading),
        CEREAL_NVP(seg.length),
        CEREAL_NVP(seg.lengthRatio),
        CEREAL_NVP(seg.M_v_accum),

        CEREAL_NVP(seg.outOfBounds),
        CEREAL_NVP(seg.isOld),
        CEREAL_NVP(seg.isDead),
        CEREAL_NVP(seg.isTooMassive)
    );
}

}

#endif