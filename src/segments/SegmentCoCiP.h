#ifndef SEGMENTCOCIP_H
#define SEGMENTCOCIP_H

#ifdef WITH_COCIP

#include <CoCiP++/CoCiP.h>
#include <CoCiP++/met.h>
#include "Segment.h"

// Forward declaration
struct Params;

// Derived CoCiP segment type
struct SegmentCoCiP : public Segment {
    CoCiP<ArrayMet<float>> cocip;
    bool doneFormation = false;

    // Empty constructor
    SegmentCoCiP() : Segment() {}

    // Constructor
    SegmentCoCiP(const FlightInputs& flightInputs, std::shared_ptr<Domain> domain,
        std::shared_ptr<Params> params);

    void evolve(const CMTime& timeStepStart, const CMTime& timeStepEnd) override;

    void dump() override;

    void addToQIcontrail() override;

private:
    // Update CoCiP's internal local meteorology; return false if interpolation requirement
    // is not satisfied (i.e. too low altitude)
    bool updateMet();
};

#endif

#endif