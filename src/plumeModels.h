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
    float m_ice = 0; // kg

    void integrate(const CMTime& timeStepStart, const CMTime& timeStepEnd) override;

    void dump() override;

    void addToQIcontrail() override;

    void formation();
};

#endif