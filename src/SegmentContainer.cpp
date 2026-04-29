#include <algorithm>
#include <functional>
#include <fstream>
#include <unordered_map>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include "SegmentContainer.h"
#include "serialization/SerializeSegment.h"
#include "serialization/SerializeCaCE.h"
#ifdef WITH_COCIP
#include "serialization/SerializeCoCiP.h"
#endif
#include "CMLog.h"

// Types to compile
template struct SegmentContainer<SegmentCaCE>;
#ifdef WITH_COCIP
template struct SegmentContainer<SegmentCoCiP>;
#endif

template<typename SegmentType>
void SegmentContainer<SegmentType>::evolvePlumes(const CMTime& startTime, const CMTime& stopTime) {
    // Using schedule(guided) to balance work when segments take different computational time
    // to evolve
    #pragma omp parallel for schedule(guided)
    for (SegmentType& seg : vec) {
        // Check segment is in bounds in case findDependentLocs failed to find centre
        if (!seg.outOfBounds) {
            seg.evolve(startTime, stopTime);
        }
    }
}

template<typename SegmentType>
void SegmentContainer<SegmentType>::advectSegments(const CMTime& startTime,
    const CMTime& stopTime) {
    
    CM_LogWrite("Advecting segments");
    
    size_t numOOB = 0;
    #pragma omp parallel for reduction(+ : numOOB)
    for (SegmentType& seg : vec) {
        seg.advect(startTime, stopTime);
        if (seg.outOfBounds) {
            numOOB += 1;
        }
    }

    CM_LogWrite(std::format("Number out of bounds: {}", numOOB));

    std::erase_if(
        vec,
        [](const SegmentType& seg) {
            return seg.outOfBounds;
        }
    );
}

template<typename SegmentType>
void SegmentContainer<SegmentType>::dump(const CMTime& stopTime) {
    CM_LogWrite("Dumping segments");
    
    // Flag old segments
    flagOldSegments(stopTime);

    // Flag large segments
    flagLargeSegments();

    size_t numOld = 0, numLarge = 0, numDead = 0;
    #pragma omp parallel for reduction(+ : numOld, numLarge, numDead)
    for (const SegmentType& seg : vec) {
        if (seg.isOld) {
            numOld++;
        }
        if (seg.isTooLarge) {
            numLarge++;
        }
        if (seg.isDead) {
            numDead++;
        }
    }

    CM_LogWrite(std::format("Number of old: {}, number of large: {}, number of dead: {}",
        numOld, numLarge, numDead));

    if (domain->twoWayCoupling) {
        // Dump flagged segments
        #pragma omp parallel for
        for (SegmentType& seg : vec) {
            if (seg.shouldBeDumped()) {
                seg.dump();
            }
        }
    }

    size_t numBefore = vec.size();

    // Remove flagged segments
    std::erase_if(
        vec,
        [](const SegmentType& seg) {
            return (seg.shouldBeDumped());
        }
    );
    
    size_t numAfter = vec.size();
    
    CM_LogWrite(std::format("Number removed: {}", numBefore - numAfter));
}

template<typename SegmentType>
void SegmentContainer<SegmentType>::constructREIcontrail() {
    struct NumDenom {
        double num = 0;
        double denom = 0;
    };
    // Map of grid cell indices to numerator/denominator values
    std::unordered_map<IDX<3, int>, NumDenom, IDXHasher<3, int>> numDenomMap;

    for (SegmentType& seg : vec) {
        const double mass = seg.totalIceMass();
        const double r_e = seg.effectiveRadius();
        // Ignore segments below threshold to avoid dividing by zero
        if (r_e < 1e-7) {
            continue;
        }

        // Add to values at segment centre's grid cell
        const IDX<3, int> ijk = domain->loc_to_ijk(seg.centre);
        auto& nd = numDenomMap[ijk];
        nd.num += mass;
        nd.denom += mass / r_e;
    }

    // Could parallelise if map is replaced with a vector
    for (const auto& [ijk, nd] : numDenomMap) {
        const double r_e_comb = (nd.denom > 0) ? nd.num / nd.denom : 0;
        domain->REIcontrail.set(ijk, r_e_comb);
    }
}

template<typename SegmentType>
void SegmentContainer<SegmentType>::save(const CMTime& currTime,
    const PlumeModels::Model plumeModel
) {
    if (vec.empty()) {
        CM_LogWrite("No live segments - will not write to file.");
        return;
    }

    std::string filename = "segments_" + currTime.asFileFriendlyString() + ".bin";

    CM_LogWrite("Saving segments to " + filename);

    {
        std::ofstream os(filename, std::ios::binary);
        if (!os) {
            CM_RaiseError("Cannot open file for writing: " + filename, __FILE__, __LINE__);
        }
        cereal::BinaryOutputArchive archive(os);
        // Write plume model ID
        archive(plumeModel.ID);
        // Then vector
        archive(vec);
    }
}

template<typename SegmentType>
void SegmentContainer<SegmentType>::load(const CMTime& time,
    const PlumeModels::Model plumeModel
) {
    std::string filename = "segments_" + time.asFileFriendlyString() + ".bin";

    CM_LogWrite("Loading segments from " + filename);

    int loadedPlumeModelID;

    {
        std::ifstream is(filename, std::ios::binary);
        if (!is) {
            CM_RaiseError("Cannot open file for reading: " + filename, __FILE__, __LINE__);
        }
        cereal::BinaryInputArchive archive(is);
        archive(loadedPlumeModelID);

        if (loadedPlumeModelID != plumeModel.ID) {
            CM_RaiseError(
                std::format(
                    "File was written with plume model ID {}, but config specifies {}",
                    loadedPlumeModelID, plumeModel.name
                ),
                __FILE__, __LINE__
            );
        }

        archive(vec);
    }

    size_t numLoaded = vec.size();

    CM_LogWrite(std::format("Loaded {} segments from file.", numLoaded));

    // Set domain pointer for each segment, then check if out of bounds
    #pragma omp parallel for
    for (SegmentType& seg : vec) {
        seg.domain = domain;

        IDX<3, int> ijk;
        if (!(domain->loc_to_ijk(seg.centre, ijk)
              && domain->can_do_interp(seg.front)
              && domain->can_do_interp(seg.back))) {
            seg.outOfBounds = true;
        }
    }

    std::erase_if(
        vec,
        [](const SegmentType& seg) {
            return seg.outOfBounds;
        }
    );

    size_t numAfter = vec.size();

    CM_LogWrite(std::format("Number out of bounds: {}", numLoaded - numAfter));

    // Find highest ID
    uint64_t maxID = 0;
    if (!vec.empty()) {
        auto it = std::ranges::max_element(vec, {}, &SegmentType::ID);
        maxID = it->ID;
    }
    // Set Segment struct ID counter to the next value
    Segment::setIDCounter(maxID + 1);
}