#ifndef SEGMENT_H
#define SEGMENT_H

#include <vector>
#include <string>
#include "timekeeping.h"
#include "Domain.h"
#include "mapFunctions.h"
#include "FlightInputs.h"

// Virtual contrail segment structure
struct Segment {
    std::string parentID = "none"; // ID of the flight object which created the segment
    CMTime birthTime; // Estimated time at which centre of segment was emitted
    FlightEmissions flightEmissions; // Flight emissions
    Domain* domPtr; // Pointer to the Contrail Manager's domain

    Geo3D back; // Location of back (first point created) of segment
    Geo3D front; // Location of front (last point created) of segment
    Geo3D centre; // Location of centre of segment; derived from back and front
    double heading; // Angle between segment and North increasing clockwise (degrees)
    double length; // Segment length (m)
    double lengthRatio = 1; // Ratio of old length to new length; set in advect, used in evolve
    double M_v_accum = 0; // Mass of ambient accumulated (double-counted) vapour (kg)

    bool outOfBounds = false; // Flag updated by SegmentContainer if out of domain bounds; segment is not dumped
    bool isOld = false; // Flag updated by SegmentContainer if passed age threshold; segment is dumped
    bool isDead = false; // Flag updated by plume model if below survival threshold; segment is dumped
    bool isTooMassive = false; // Flag updated by SegmentContainer if passed size threshold; segment is dumped

    // Constructor
    Segment(const FlightInputs& flightInputs, Domain* domPtr)
        : parentID(flightInputs.ID),
          birthTime(flightInputs.birthTime),
          flightEmissions(flightInputs.emissions),
          domPtr(domPtr),
          back(flightInputs.back), front(flightInputs.front) {
        
        findDependentLocs();
        length = findLength();
    }

    // Virtual destructor
    virtual ~Segment() = default;

    // Virtual integration method; must be overridden by plume model-specific method
    virtual void evolve(const CMTime& startTime, const CMTime& stopTime) = 0;

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

    // Calculate segment centre and heading
    inline void findDependentLocs() {
        centre = great_circle_interp(0.5, back, front);
        // Ensure centre is in grid
        IDX3<int> ijkCentre;
        if (!domPtr->loc_to_ijk(centre, ijkCentre)) {
            outOfBounds = true;
        }
        heading = great_circle_bearing(back, front);
    }

    // Calculate (but not update) segment length
    constexpr double findLength() {
        return great_circle_dist(back, front);
    }
    
    // Advect front and back locations of segment and flag outOfBounds if out of bounds
    void advect(const CMTime& startTime, const CMTime& stopTime) {
        float duration_s;
        // If birthTime > startTime, evolve from birthTime to stopTime
        duration_s = (birthTime > startTime)
            ? (stopTime - birthTime).to_s()
            : (stopTime - startTime).to_s();

        bool inGridBack, inGridFront;

        inGridBack = advect_loc_RK4(back, duration_s, *domPtr);
        inGridFront = advect_loc_RK4(front, duration_s, *domPtr);

        // If either end of segment has drifted out of grid, remove segment 
        if (!(inGridBack && inGridFront)) {
            outOfBounds = true;
        }

        // Find new length
        double newLength = findLength();
        lengthRatio = length / newLength;
        length = newLength;

        // Update dependent locs
        findDependentLocs();
    }
};

#endif