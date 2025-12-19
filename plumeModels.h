#ifndef PLUMEMODELS
#define PLUMEMODELS

#include <string>
#include "domain.h"
#include "segment.h"
#include "timekeeping.h"

// Plume model IDs

const int MODEL_ID_BASIC_PLUME = 1;

// Plume model names

const std::string MODEL_STR_BASIC_PLUME = "Basic plume model";


struct SegmentBasicPlume : public Segment {
    // Plume model-specific data
    float n_ice = 0;

    SegmentBasicPlume(Domain& dom) : Segment(dom) {}

    // Plume model-specific integration method
    void integrate(const CMTime& timeStepStart, const CMTime& timeStepEnd) override {
        n_ice = 10;
    }
};

#endif