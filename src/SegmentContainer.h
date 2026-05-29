#ifndef SEGMENTCONTAINER_H
#define SEGMENTCONTAINER_H

#include <vector>
#include <memory>
#include <variant>
#include <omp.h>
#include "PlumeModels.h"
#include "domain/Domain.h"
#include "map/types.h"
#include "timekeeping.h"
#include "segment/SegmentCaCE.h"
#ifdef WITH_COCIP
#include <CoCiP++/params.h>
#include "segment/SegmentCoCiP.h"
#endif

// Plume model-specific segment container structure
// Holds segments in a private vector of SegmentType
template <typename SegmentType>
struct SegmentContainer {
    // Maximum age of a contrail segment (s); set by ContrailManager
    double maxSegmentAge_s;
    // Maximum length of a contrail segment at any time (m); set by ContrailManager
    double maxSegLen;
    // Maximum ratio of double-counted water vapour mass in contrail plume to water vapour mass in
    // grid cell (); set by ContrailManager
    double maxAccumVapRatio;

    Domain* domain; // Pointer to the Contrail Manager's domain

#ifdef WITH_COCIP
    std::shared_ptr<Params> cocipParams; // Pointer to CoCiP Params object if using
#endif

private:
    // Vector of new
    std::vector<SegmentType> newSegments;
    // Vector of segments
    std::vector<SegmentType> vec;

    // Returns a new object of SegmentType; allows plume model-specific initialisation
    inline SegmentType newSegmentInstance(const FlightInputs& flightInputs);

    // Update isOld flag for each segment if past age threshold (maxSegmentAge_s) at a given time
    // (parallelised)
    void flagOldSegments(const CMTime& time) {
        #pragma omp parallel for
        for (SegmentType& seg : vec) {
            if ((time - seg.birthTime).to_s() > maxSegmentAge_s) {
                seg.isOld = true;
            }
        }
    }

    // Updates isTooLarge flag for each segment if past size threshold (parallelised)
    void flagLargeSegments() {
        #pragma omp parallel for
        for (SegmentType& seg : vec) {
            IDX<3, int> ijkCurr = domain->loc_to_ijk(seg.centre);

            double gridVapourMass = domain->QV.get(ijkCurr)
                                    * domain->DRYMASS.get(ijkCurr);

            if ((seg.length > maxSegLen)
                || (seg.M_v_accum / gridVapourMass > maxAccumVapRatio)) {
                seg.isTooLarge = true;
            }
        }
    }

public:
    // Return number of segments in container
    size_t getSize() const {
        return vec.size();
    }

    // Add segment to vector of new segments; assumes segment has been checked to be inside domain
    void addNewSegment(const FlightInputs& flightInputs) {
        // Add to vector
        #pragma omp critical
        {
            newSegments.push_back(newSegmentInstance(flightInputs));
        }
    }

    // Run the formation method of each of the new segments (parallelised)
    // Segments which form contrails are added to vec; the rest are erased and newSegments is reset
    void runFormation();

    // Advect all segments and remove if out of bounds (parallelised)
    void advectSegments(const CMTime& startTime, const CMTime& stopTime);

    // Evolve all segment plumes (parallelised)
    void evolvePlumes(const CMTime& startTime, const CMTime& stopTime);

    // Dump old, large, or dead segments (parallelised)
    void dump(const CMTime& stopTime);

    // Construct QIcontrail using live segment data (parallelised)
    void constructQIcontrail() {
        #pragma omp parallel for
        for (SegmentType& seg : vec) {
            seg.addToQIcontrail();
        }
    }

    // Construct REIcontrail using live segment data (not parallelised)
    void constructREIcontrail();

    // Save segments to file with currTime in name
    void save(const CMTime& currTime, const PlumeModels::Model plumeModel);

    // Load segments from file with time in name
    void load(const CMTime& time, const PlumeModels::Model plumeModel);
};

template <>
inline SegmentCaCE SegmentContainer<SegmentCaCE>::newSegmentInstance(
    const FlightInputs& flightInputs
) {
    // Initialise with additional pointer to common params
    return SegmentCaCE(flightInputs, domain);
}

#ifdef WITH_COCIP
template <>
inline SegmentCoCiP SegmentContainer<SegmentCoCiP>::newSegmentInstance(
    const FlightInputs& flightInputs
) {
    // Initialise with additional pointer to common params
    return SegmentCoCiP(flightInputs, domain, cocipParams);
}
#endif

// Define SegmentContainer variant alias
using SegmentContainerVariant = std::variant<
    SegmentContainer<SegmentCaCE>,
    SegmentContainer<SegmentCoCiP>
>;

#endif