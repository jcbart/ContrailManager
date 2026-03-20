#ifndef SEGMENTCONTAINER_H
#define SEGMENTCONTAINER_H

#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <memory>
#include <omp.h>
#ifdef WITH_COCIP
#include <CoCiP++/CoCiP.h>
#include <CoCiP++/params.h>
#include "SegmentCoCiP.h"
#endif
#include "Domain.h"
#include "mapTypes.h"
#include "timekeeping.h"
#include "FlightInputs.h"
#include "CMLog.h"

// Virtual contrail segment container structure
struct ISegmentContainer {
    // Maximum age of a contrail segment (s); set by ContrailManager
    float maxContrailAge_s;
    // Maximum ratio of double-counted water vapour mass in contrail plume to water vapour mass in
    // grid cell (); set by ContrailManager
    float maxAccumVapRatio;

    Domain* domPtr = nullptr; // Pointer to the Contrail Manager's domain

#ifdef WITH_COCIP
    std::shared_ptr<Params> cocipParams; // Pointer to CoCiP Params object if using
#endif

    // Virtual destructor
    virtual ~ISegmentContainer() = default;

    // Return number of segments in container
    virtual size_t getSize() const = 0;

    // Add segment to container; assumes segment has been checked to be inside domain
    virtual void addItem(const std::string& parentID, const CMTime& birthTime,
        const FlightInputs& flightInputs, const Geo3D& backLoc, const Geo3D& frontLoc,
        const float length) = 0;
    
    // Evolve all segment plumes
    virtual void evolvePlumes(const CMTime& startTime, const CMTime& stopTime) = 0;

    // Advect all segments and remove if out of bounds
    virtual void advectSegments(const CMTime& startTime, const CMTime& stopTime) = 0;

    // Dump old, massive, or dead segments
    virtual void dump(const CMTime& stopTime) = 0;

    // Construct QIcontrail using live segment data
    virtual void constructQIcontrail() = 0;
};

// Plume model-specific segment container structure
// Holds segments in a private vector of SegmentType
template <typename SegmentType>
struct SegmentContainer : public ISegmentContainer {
private:
    std::vector<SegmentType> vec;

    // Returns a new object of SegmentType; allows plume model-specific initialisation
    inline SegmentType newSegmentInstance(const std::string& parentID, const CMTime& birthTime,
        const FlightInputs& flightInputs, const Geo3D& backLoc, const Geo3D& frontLoc,
        const float length);

    // Update isOld flag for each segment if past age threshold (maxContrailAge_s) at a given time
    // (parallelised)
    void flagOldSegments(const CMTime& time) {
        #pragma omp parallel for
        for (SegmentType& seg : vec) {
            if ((time - seg.birthTime).to_s() > maxContrailAge_s) {
                seg.isOld = true;
            }
        }
    }

    // Updates isTooMassive flag for each segment if past size threshold (parallelised)
    void flagTooMassiveSegments() {
        #pragma omp parallel for
        for (SegmentType& seg : vec) {
            IDX3<int> ijkCurr = domPtr->loc_to_ijk(seg.centre);

            double gridVapourMass = domPtr->QV.get(ijkCurr)
                                    * domPtr->DRYMASS.get(ijkCurr);

            if (seg.M_v_accum / gridVapourMass > maxAccumVapRatio) {
                seg.isTooMassive = true;
            }
        }
    }

public:
    // Return number of segments in container
    size_t getSize() const override {
        return vec.size();
    }

    // Add segment to container; assumes segment has been checked to be inside domain
    void addItem(const std::string& parentID, const CMTime& birthTime,
        const FlightInputs& flightInputs, const Geo3D& backLoc, const Geo3D& frontLoc,
        const float length) override {

        SegmentType newSeg = newSegmentInstance(
            parentID,
            birthTime,
            flightInputs,
            backLoc,
            frontLoc,
            length
        );

        // Add to vector; use move to support segments which have unique_ptr
        #pragma omp critical
        {
            vec.push_back(std::move(newSeg));
        }

        CM_LogWrite("Segment created with birth time: " + newSeg.birthTime.asString());
        CM_LogWrite("Centre location: " + newSeg.centre.asString());
        CM_LogWrite("Length: " + std::to_string(newSeg.length));
    }

    // Evolve all segment plumes (parallelised)
    void evolvePlumes(const CMTime& startTime, const CMTime& stopTime) override {
        // Using schedule(guided) to balance work when segments take different computational time
        // to evolve
        #pragma omp parallel for schedule(guided)
        for (SegmentType& seg : vec) {
            seg.evolve(startTime, stopTime);
        }
    }

    // Advect all segments and remove if out of bounds (parallelised)
    void advectSegments(const CMTime& startTime, const CMTime& stopTime) override {
        CM_LogWrite("Advecting segments");
        
        size_t numOOB = 0;
        #pragma omp parallel for reduction(+ : numOOB)
        for (SegmentType& seg : vec) {
            seg.advect(startTime, stopTime);
            if (seg.outOfBounds) {
                numOOB += 1;
            }
        }

        CM_LogWrite("Number out of bounds: " + std::to_string(numOOB));

        std::erase_if(
            vec,
            [](const SegmentType& seg) {
                return seg.outOfBounds;
            }
        );
    }

    // Dump old, massive, or dead segments (parallelised)
    void dump(const CMTime& stopTime) override {
        CM_LogWrite("Dumping old, massive, and dead segments");
        
        // Flag old segments
        flagOldSegments(stopTime);

        // Flag massive segments
        flagTooMassiveSegments();

        size_t numOld = 0, numMassive = 0, numDead = 0;
        #pragma omp parallel for reduction(+ : numOld, numMassive, numDead)
        for (const SegmentType& seg : vec) {
            if (seg.isOld) {
                numOld++;
            }
            if (seg.isTooMassive) {
                numMassive++;
            }
            if (seg.isDead) {
                numDead++;
            }
        }

        CM_LogWrite("Number of old: " + std::to_string(numOld) + ", number of massive: "
            + std::to_string(numMassive) + ", number of dead: " + std::to_string(numDead));

        if (domPtr->twoWayCoupling) {
            // Dump if old, massive, or dead
            #pragma omp parallel for
            for (SegmentType& seg : vec) {
                if (seg.shouldBeDumped()) {
                    seg.dump();
                }
            }
        }

        size_t numBefore = vec.size();

        // Remove old or dead
        std::erase_if(
            vec,
            [](const SegmentType& seg) {
                return (seg.shouldBeDumped());
            }
        );
        
        size_t numAfter = vec.size();
        
        CM_LogWrite("Number of old/massive/dead: " + std::to_string(numBefore - numAfter));
    }

    // Construct QIcontrail using live segment data (parallelised)
    void constructQIcontrail() override {
        #pragma omp parallel for
        for (SegmentType& seg : vec) {
            seg.addToQIcontrail();
        }
    }
};

#ifdef WITH_COCIP
template <>
inline SegmentCoCiP SegmentContainer<SegmentCoCiP>::newSegmentInstance(const std::string& parentID,
    const CMTime& birthTime, const FlightInputs& flightInputs, const Geo3D& backLoc,
    const Geo3D& frontLoc, const float length) {
    
    // Initialise with additional pointer to common params
    SegmentCoCiP newSeg(parentID, birthTime, flightInputs, domPtr, backLoc, frontLoc, length,
        cocipParams);
    return newSeg;
}
#endif

#endif