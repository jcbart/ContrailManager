#ifndef SEGMENTCOCIP_H
#define SEGMENTCOCIP_H

#include <string>
#include <memory>
#ifdef WITH_COCIP
#include <CoCiP++/CoCiP.h>
#include <CoCiP++/met.h>
#endif
#include "Segment.h"

// Derived segment struct

#ifdef WITH_COCIP
// Forward declaration
struct Params;

// Derived CoCiP segment type
struct SegmentCoCiP : public Segment {
    CoCiP<ArrayMet<float>> cocip;
    bool doneFormation = false;

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