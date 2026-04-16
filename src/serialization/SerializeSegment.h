#ifndef SERIALIZESEGMENT_H
#define SERIALIZESEGMENT_H

#include <cereal/types/memory.hpp>
#include "Segment.h"
#include "timekeeping.h"
#include "map/types.h"
#include "FlightInputs.h"

namespace cereal {

template<class Archive>
void serialize(Archive& ar, CMTime& datetime) {
    ar(
        datetime.timepoint
    );
}

template<class Archive>
void serialize(Archive& ar, Geo3D& loc) {
    ar(
        loc.lon,
        loc.lat,
        loc.alt
    );
}

template<class Archive>
void serialize(Archive& ar, FlightEmissions& emissions) {
    ar(
        emissions.engine_efficiency,
        emissions.ei_h2o,
        emissions.q_fuel,
        emissions.aircraft_mass,
        emissions.wingspan,
        emissions.true_airspeed,
        emissions.fuel_flow,
        emissions.nvpm_ei_n
    );
}

template<class Archive>
void serialize(Archive& ar, Segment& seg) {
    ar(
        seg.ID,
        seg.parentID,
        seg.birthTime,
        seg.flightEmissions,
        // domain is not serialized; assigned upon restart

        seg.back,
        seg.front,
        seg.centre,
        seg.heading,
        seg.length,
        seg.lengthRatio,
        seg.M_v_accum,

        seg.outOfBounds,
        seg.isOld,
        seg.isDead,
        seg.isTooMassive
    );
}

}

#endif