#ifndef SEGMENTCOCIP_H
#define SEGMENTCOCIP_H

#include <string>
#ifdef WITH_COCIP
#include <CoCiP++/CoCiP.h>
#include <CoCiP++/met.h>
#endif
#include "Segment.h"

// Forward declarations

struct Params;

// Plume model ID

const int MODEL_ID_COCIP = 1;

// Plume model name

const std::string MODEL_STR_COCIP = "CoCiP";

// Derived segment struct

#ifdef WITH_COCIP
// Derived CoCiP segment type
struct SegmentCoCiP : public Segment {
    CoCiP<ArrayMet<float>> cocip;
    bool doneFormation = false;

    // Constructor
    SegmentCoCiP(const std::string& parentID, const CMTime& birthTime,
        const FlightInputs& flightInputs, Domain* domPtr, const Geo3D& backLoc,
        const Geo3D& frontLoc, const float length, Params* params);

    // Update CoCiP's internal local meteorology
    void updateMet();

    void integrate(const CMTime& timeStepStart, const CMTime& timeStepEnd) override;

    void dump() override;

    void addToQIcontrail() override;
};
#endif

#endif