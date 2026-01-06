#ifndef SEGMENT_H
#define SEGMENT_H

#include <vector>
#include <string>
#include "timekeeping.h"
#include "domain.h"
#include "mapUtils.h"

// Virtual contrail segment structure
struct Segment {
    std::string parentID = "none"; // ID of the flight object which created the segment
    CMTime birthTime; // Estimated time at which centre of segment was emitted
    Geo3D back; // Location of back (first point created) of segment
    Geo3D front; // Location of front (last point created) of segment
    Geo3D centre; // Location of centre of segment; derived from back and front

    float length; // Segment length (m)

    // Pointer to the Contrail Manager's domain
    Domain* domPtr;

    bool outOfBounds = false;
    bool isOld = false;
    bool isDead = false;

    // Virtual destructor
    virtual ~Segment() = default;

    // Virtual integration method; must be overridden by plume model-specific method
    virtual void integrate(const CMTime& timeStepStart, const CMTime& timeStepEnd) = 0;

    // Virtual method to add the "contents" of the segment into the NWP's native fields
    // before it is destroyed
    virtual void dump() = 0;

    // Virtual method to add the contrail ice mass in the segment to the QIcontrail field
    virtual void addToQIcontrail() = 0;

    void find_dependent_locs() {
        centre = great_circle_interp(0.5, back, front);
    }
    
    void advect(const CMTime& timeStepStart, const CMTime& timeStepEnd) {
        float duration_s;
        // If birthTime > timeStepStart, integrate from birthTime to timeStepEnd
        if (birthTime > timeStepStart) {
            duration_s = (timeStepEnd - birthTime).dhms_to_s();
        }
        else {
            duration_s = (timeStepEnd - timeStepStart).dhms_to_s();
        }
        bool inGridBack, inGridFront;

        inGridBack = advect_loc_RK4(back, duration_s, *domPtr);
        inGridFront = advect_loc_RK4(front, duration_s, *domPtr);

        // If either end of segment has drifted out of grid, remove segment 
        if (!(inGridBack && inGridFront)) {
            outOfBounds = true;
        }
        float newLength = great_circle_dist(back, front);
        // width *= length/newLength
        length = newLength;
        find_dependent_locs();
    }
};

#endif