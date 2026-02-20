#ifndef SEGMENTCONTAINER_H
#define SEGMENTCONTAINER_H

#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <ESMC.h>
#ifdef WITH_COCIP
#include <CoCiP++/CoCiP.h>
#include <CoCiP++/params.h>
#include "SegmentCoCiP.h"
#endif
#include "Domain.h"
#include "mapUtils.h"
#include "timekeeping.h"
#include "FlightInputs.h"

// Virtual contrail segment container structure
struct ISegmentContainer {
    // Maximum age of a contrail segment (s); set by ContrailManager
    float maxContrailAge_s;
    // Maximum ratio of double-counted water vapour mass in contrail plume to water vapour mass in
    // grid cell (); set by ContrailManager
    float maxAccumVapRatio;

    IDomain* domPtr = nullptr; // Pointer to the Contrail Manager's domain

#ifdef WITH_COCIP
    Params* cocipParams = nullptr; // Pointer to CoCiP Params object if using
#endif

    // Virtual destructor
    virtual ~ISegmentContainer() = default;

    // Return number of segments in container
    virtual size_t getSize() const = 0;

    // Add segment to container; assumes segment has been checked to be inside domain
    virtual void addItem(const std::string& parentID, const CMTime& birthTime,
        const FlightInputs& flightInputs, const Geo3D& backLoc, const Geo3D& frontLoc,
        const float length) = 0;
    
    // Evolve all segment plumes in vector
    virtual void integratePlumes(const CMTime& timeStepStart, const CMTime& timeStepEnd) = 0;

    // Dump old, massive, or dead segments
    virtual void dump(const CMTime& timeStepEnd) = 0;

    // Advect all segments in vector and remove if out of bounds
    virtual void advectSegments(const CMTime& timeStepStart, const CMTime& timeStepEnd) = 0;

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

    // Update isOld flag for each segment if past age threshold (maxContrailAge_s) at a given
    // time
    void flagOldSegments(const CMTime& time) {
        for (SegmentType& seg : vec) {
            if ((time - seg.birthTime).dhms_to_s() > maxContrailAge_s) {
                seg.isOld = true;
            }
        }
    }

    // Updates isTooMassive flag for each segment if past size threshold
    void flagTooMassiveSegments() {
        for (SegmentType& seg : vec) {
            IDX3<int> ijkCurr;
            // Should be safe to ignore return value
            bool inGrid = domPtr->loc_to_ijk(seg.centre, ijkCurr);

            double gridVapourMass = domPtr->QV.get_value(ijkCurr)
                                    * domPtr->DRYMASS.get_value(ijkCurr);

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

        vec.push_back(newSeg);
        
        int rc;
        std::string msg;
        msg = "Segment created with birth time: " + newSeg.birthTime.asString();
        rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
        msg = "Centre location: " + newSeg.centre.asString();
        rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
        msg = "Length: " + std::to_string(newSeg.length);
        rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
    }

    // Evolve all segment plumes in vector
    void integratePlumes(const CMTime& timeStepStart, const CMTime& timeStepEnd) override {
        for (SegmentType& seg : vec) {
            seg.integrate(timeStepStart, timeStepEnd);
        }
    }

    // Dump old, massive, or dead segments
    void dump(const CMTime& timeStepEnd) override {
        int rc;
        std::string msg;
        rc = ESMC_LogWrite("Dumping old and dead segments", ESMC_LOGMSG_INFO);
        
        // Flag old segments
        flagOldSegments(timeStepEnd);

        // Flag massive segments
        flagTooMassiveSegments();

        size_t numOld = 0, numMassive = 0, numDead = 0;
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

        msg = "Number of old: " + std::to_string(numOld) + ", number of massive: " + std::to_string(numMassive) + ", number of dead: " + std::to_string(numDead);
        rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);

        if (domPtr->twoWayCoupling) {
            // Dump if old, massive, or dead
            for (SegmentType& seg : vec) {
                if (seg.shouldBeDumped()) {
                    seg.dump();
                }
            }
        }

        size_t numBefore = vec.size();

        // Remove old or dead
        vec.erase(std::remove_if(vec.begin(), vec.end(),
                                 [](const SegmentType& seg) {
                                    return (seg.shouldBeDumped());
                                 }), vec.end());
        
        size_t numAfter = vec.size();
        
        msg = "Number of old/massive/dead: " + std::to_string(numBefore - numAfter);
        rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
    }

    // Advect all segments in vector and remove if out of bounds
    void advectSegments(const CMTime& timeStepStart, const CMTime& timeStepEnd) override {
        int rc;
        std::string msg;
        rc = ESMC_LogWrite("Advecting segments", ESMC_LOGMSG_INFO);
        size_t numOOB = 0;
        for (SegmentType& seg : vec) {
            seg.advect(timeStepStart, timeStepEnd);
            if (seg.outOfBounds) {
                numOOB += 1;
            }
        }
        msg = "Number out of bounds: " + std::to_string(numOOB);
        rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);

        vec.erase(std::remove_if(vec.begin(), vec.end(),
                                 [](const SegmentType& seg) {
                                    return seg.outOfBounds;
                                 }), vec.end());
    }

    // Construct QIcontrail using live segment data
    void constructQIcontrail() override {
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
    SegmentCoCiP newSeg(parentID, birthTime, flightInputs, domPtr, backLoc, frontLoc, length, cocipParams);
    return newSeg;
}
#endif

#endif