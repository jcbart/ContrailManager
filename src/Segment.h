#ifndef SEGMENT_H
#define SEGMENT_H

#include <vector>
#include <string>
#include "timekeeping.h"
#include "Domain.h"
#include "mapUtils.h"
#include "FlightInputs.h"

// Virtual contrail segment structure
struct Segment {
    std::string parentID = "none"; // ID of the flight object which created the segment
    CMTime birthTime; // Estimated time at which centre of segment was emitted
    FlightInputs flightInputs; // Inputs from flight
    IDomain* domPtr; // Pointer to the Contrail Manager's domain

    Geo3D back; // Location of back (first point created) of segment
    Geo3D front; // Location of front (last point created) of segment
    Geo3D centre; // Location of centre of segment; derived from back and front
    double heading; // Angle between segment and North (degrees)
    double length; // Segment length (m)
    double lengthRatio = 1; // Ratio of old length to new length; set in advect, used in integrate
    double M_v_accum = 0; // Mass of ambient accumulated (double-counted) vapour (kg)

    bool outOfBounds = false; // Flag updated by SegmentContainer if out of domain bounds; segment is not dumped
    bool isOld = false; // Flag updated by SegmentContainer if passed age threshold; segment is dumped
    bool isDead = false; // Flag updated by plume model if below survival threshold; segment is dumped
    bool isTooMassive = false; // Flag updated by SegmentContainer if passed size threshold; segment is dumped

    // Constructor
    Segment(const std::string& parentID, const CMTime& birthTime, const FlightInputs& flightInputs,
        IDomain* domPtr, const Geo3D& backLoc, const Geo3D& frontLoc, const float length)
        : parentID(parentID), birthTime(birthTime), flightInputs(flightInputs), domPtr(domPtr),
          back(backLoc), front(frontLoc), length(length) {
        
        find_dependent_locs();
    }

    // Virtual destructor
    virtual ~Segment() = default;

    // Virtual integration method; must be overridden by plume model-specific method
    virtual void integrate(const CMTime& timeStepStart, const CMTime& timeStepEnd) = 0;

    // Returns true if segment should be dumped (i.e. if flags raised for isOld, isTooMassive, or
    // isDead)
    constexpr bool shouldBeDumped() const {
        return (isOld || isDead || isTooMassive);
    }

    // Virtual method to add the "contents" of the segment into the NWP's native fields
    // before it is destroyed. Only called if two-way coupling.
    virtual void dump() = 0;

    // Virtual method to add the contrail ice mass in the segment to the QIcontrail field
    virtual void addToQIcontrail() = 0;

    inline void find_dependent_locs() {
        centre = great_circle_interp(0.5, back, front);
        heading = great_circle_bearing(back, front);
    }
    
    // Advect front and back locations of segment and flag outOfBounds if out of bounds
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

        // Find new length
        double newLength = great_circle_dist(back, front);
        lengthRatio = length / newLength;
        length = newLength;

        // Update dependent locs
        find_dependent_locs();
    }
};

#endif