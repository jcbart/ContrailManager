#ifndef PLUMEMODELS_H
#define PLUMEMODELS_H

#include <string>
#include "segment.h"

// Plume model IDs

const int MODEL_ID_BASIC_PLUME = 1;

// Plume model names

const std::string MODEL_STR_BASIC_PLUME = "Basic plume model";

// Derived segment structs

struct SegmentBasicPlume : public Segment {
    // Plume model-specific data
    bool doneFormation = false;
    float cross_section_area = 0; // (m2)
    float r_ice; // (m)
    float n_ice = 0; // (# m-3)
    float N_ice_per_m = 0; // (# m-1)
    float M_ice_per_m = 0; // (kg m-1)

    void integrate(const CMTime& timeStepStart, const CMTime& timeStepEnd) override;

    void dump() override;

    void addToQIcontrail() override;

    void scaleWidthAfterAdvection(float lengthRatio) override;

    void formation();
};

#endif