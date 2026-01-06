#ifndef SEGMENTCONTAINER_H
#define SEGMENTCONTAINER_H

#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <ESMC.h>
#include "mapUtils.h"
#include "timekeeping.h"

// Virtual contrail segment container structure
struct ISegmentContainer {
    int maxContrailAge_s;

    virtual ~ISegmentContainer() = default;
    // Return number of segments in container
    virtual size_t getSize() const = 0;
    // Add segment to container after checking segment is in domain
    virtual void addItem(const std::string& parentID, const Geo3D& backLoc, const Geo3D& frontLoc,
                         const float& length, const CMTime& birthTime, Domain& domain) = 0;
    // Integrate all segment plumes in vector
    virtual void integratePlumes(const CMTime& timeStepStart, const CMTime& timeStepEnd) = 0;
    // Dump old or dead segments
    virtual void dump(const CMTime& timeStepEnd) = 0;
    // Advect all segments in vector
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

public:
    size_t getSize() const override {
        return vec.size();
    }

    void addItem(const std::string& parentID, const Geo3D& backLoc, const Geo3D& frontLoc,
                 const float& length, const CMTime& birthTime, Domain& domain) override {

        SegmentType newSeg;
        newSeg.parentID = parentID;
        newSeg.back = backLoc;
        newSeg.front = frontLoc;
        newSeg.length = length;
        newSeg.birthTime = birthTime;
        newSeg.domPtr = &domain; // Pass address to pointer
        newSeg.find_dependent_locs();
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

    void integratePlumes(const CMTime& timeStepStart, const CMTime& timeStepEnd) override {
        for (SegmentType& seg : vec) {
            seg.integrate(timeStepStart, timeStepEnd);
        }
    }

    void dump(const CMTime& timeStepEnd) override {
        int rc;
        std::string msg;
        rc = ESMC_LogWrite("Dumping old and dead segments", ESMC_LOGMSG_INFO);
        int numOldOrDead = 0;
        for (SegmentType& seg : vec) {
            // Mark if old
            if (((timeStepEnd - seg.birthTime).dhms_to_s() > maxContrailAge_s)) {
                seg.isOld = true;
            }
            // Dump if old or dead
            if (seg.isOld || seg.isDead) {
                numOldOrDead += 1;
                seg.dump();
            }
        }

        msg = "Number of old/dead: " + std::to_string(numOldOrDead);
        rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);

        // Remove old or dead
        vec.erase(std::remove_if(vec.begin(), vec.end(),
                                 [](const SegmentType& seg) {
                                    return (seg.isOld || seg.isDead);
                                 }), vec.end());
    }

    void advectSegments(const CMTime& timeStepStart, const CMTime& timeStepEnd) override {
        int rc;
        std::string msg;
        rc = ESMC_LogWrite("Advecting segments", ESMC_LOGMSG_INFO);
        int numOOB = 0;
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

    void constructQIcontrail() override {
        for (SegmentType& seg : vec) {
            seg.addToQIcontrail();
        }
    }
};

#endif