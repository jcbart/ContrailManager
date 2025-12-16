#ifndef PLUMEMODELS
#define PLUMEMODELS

#include <string>
#include "segment.h"
#include "timekeeping.h"

// Plume model IDs

const int MODEL_ID_BASIC_PLUME = 1;

// Plume model names

const std::string MODEL_STR_BASIC_PLUME = "Basic plume model";

// Plume model integration functions

//void integ_basic_plume(Segment& seg, const CMTime& timeStepStart, const CMTime& timeStepEnd);

#endif