#ifndef SERIALIZECACE_H
#define SERIALIZECACE_H

#include "segments/SegmentCaCE.h"

namespace cereal {

// Serialize SegmentCaCE
template<class Archive>
void serialize(Archive& ar, SegmentCaCE& seg) {
    // Do nothing
    // There should be no SegmentCaCE alive at the end of a coupling interval to be saved
    // This function is here to simplify the saving functionality
}

}

#endif