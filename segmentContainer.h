#ifndef SEGMENTCONTAINER
#define SEGMENTCONTAINER

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
    // Advect all segments in vector
    virtual void advectSegments(const CMTime& timeStepStart, const CMTime& timeStepEnd) = 0;
    // Dump old or dead segments
    virtual void dump(const CMTime& timeStepEnd) = 0;
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

        SegmentType newSeg(domain);
        newSeg.parentID = parentID;
        newSeg.back = backLoc;
        newSeg.front = frontLoc;
        newSeg.length = length;
        newSeg.birthTime = birthTime;
        newSeg.find_dependent_locs();
        vec.push_back(newSeg);
        
        int rc;
        std::string msg;
        msg = "Segment created with birth time: " + newSeg.birthTime.asString();
        rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
        msg = "Centre location: (" + std::to_string(newSeg.centre.lon) + ", " + std::to_string(newSeg.centre.lat) + ", " + std::to_string(newSeg.centre.alt) + ")";
        rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
        msg = "Length: " + std::to_string(newSeg.length);
        rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
    }

    void integratePlumes(const CMTime& timeStepStart, const CMTime& timeStepEnd) override {
        for (SegmentType& seg : vec) {
            seg.integrate(timeStepStart, timeStepEnd);
        }
    }

    void advectSegments(const CMTime& timeStepStart, const CMTime& timeStepEnd) override {
        for (SegmentType& seg : vec) {
            seg.advect(timeStepStart, timeStepEnd);
        }

        vec.erase(std::remove_if(vec.begin(), vec.end(),
                                 [](const SegmentType& seg) {
                                    return seg.outOfBounds;
                                 }), vec.end());
    }

    void dump(const CMTime& timeStepEnd) override {
        // Mark old
        for (SegmentType& seg : vec) {
            if ((timeStepEnd - seg.birthTime).dhms_to_s() > maxContrailAge_s) {
                // Dump (seg.dump()), then
                seg.isOld = true;
            }
        }

        // Remove old or dead
        vec.erase(std::remove_if(vec.begin(), vec.end(),
                                 [](const SegmentType& seg) {
                                    return seg.isOld;
                                 }), vec.end());
    }
};

#endif