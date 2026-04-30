#ifndef SEGMENT_H
#define SEGMENT_H

#include <vector>
#include <string>
#include <memory>
#include <atomic>
#include "timekeeping.h"
#include "Domain.h"
#include "map/functions.h"
#include "flight/FlightInputs.h"

// Virtual contrail segment structure
struct Segment {
    uint64_t ID; // Unique segment ID (not sequential if multi-threaded)
    std::string parentID; // ID of the flight object which created the segment
    CMTime birthTime; // Estimated time at which centre of segment was emitted
    FlightEmissions flightEmissions; // Flight emissions
    std::shared_ptr<Domain> domain; // Pointer to the Contrail Manager's domain

    Geo3D back; // Location of back (first point created) of segment
    Geo3D front; // Location of front (last point created) of segment
    Geo3D centre; // Location of centre of segment; derived from back and front
    double heading; // Angle between segment and North increasing clockwise (degrees)
    double length; // Segment length (m)
    double lengthRatio = 1; // Ratio of old length to new length; set in advect, used in evolve
    double M_v_accum = 0; // Mass of ambient accumulated (double-counted) vapour (kg)

     // Flag updated by SegmentContainer or plume model if out of domain bounds; segment is not dumped
    bool outOfBounds = false;
    // Flag updated by SegmentContainer if passed age threshold; segment is dumped
    bool isOld = false;
    // Flag updated by Segment or plume model if below survival threshold; segment is dumped
    bool isDead = false;
    // Flag updated by SegmentContainer if past size threshold; segment is dumped
    bool isTooLarge = false;

    // Empty constructor
    Segment() {}

    // Constructor
    Segment(const FlightInputs& flightInputs, std::shared_ptr<Domain> domain)
        : ID(nextID()),
          parentID(flightInputs.ID),
          birthTime(flightInputs.birthTime),
          flightEmissions(flightInputs.emissions),
          domain(domain),
          back(flightInputs.back),
          front(flightInputs.front) {
        
        findDependentLocs();
        length = calcLength();
    }

    // Virtual destructor
    virtual ~Segment() = default;

    // Returns true if segment should be dumped (i.e. if flags raised for isOld, isTooLarge, or
    // isDead)
    constexpr bool shouldBeDumped() const {
        return (isOld || isDead || isTooLarge);
    }

    // Virtual method to return total contrail ice mass (kg)
    virtual double totalIceMass() const = 0;

    // Virtual method to return contrail ice effective radius (m)
    virtual double effectiveRadius() const = 0;

    // Virtual integration method; must be overridden by plume model-specific method
    virtual void evolve(const CMTime& startTime, const CMTime& stopTime) = 0;

    // Virtual method to add the "contents" of the segment into the NWP's native fields
    // before it is destroyed. Only called if two-way coupling.
    virtual void dump() = 0;

    // Virtual method to add the contrail ice mass in the segment to the QIcontrail field
    virtual void addToQIcontrail() = 0;

    // Calculate segment centre and heading
    inline void findDependentLocs() {
        centre = map::great_circle_interp(0.5, back, front);
        // Ensure centre is in grid
        IDX<3, int> ijkCentre;
        if (!domain->loc_to_ijk(centre, ijkCentre)) {
            outOfBounds = true;
        }
        heading = map::great_circle_bearing(back, front);
    }

    // Calculate (but not update) segment length
    constexpr double calcLength() {
        return map::great_circle_dist(back, front);
    }
    
    // Advect front and back locations of segment and flag outOfBounds if out of bounds
    void advect(const CMTime& startTime, const CMTime& stopTime) {
        float duration_s;
        // If birthTime > startTime, evolve from birthTime to stopTime
        duration_s = (birthTime > startTime)
            ? (stopTime - birthTime).to_s()
            : (stopTime - startTime).to_s();

        // If either end of segment has drifted out of grid, remove segment 
        if (!(map::advect_loc_RK4(back, duration_s, *domain)
              && map::advect_loc_RK4(front, duration_s, *domain)))
        {
            outOfBounds = true;
        }

        // Find new length
        double newLength = calcLength();
        // Set a minimum on length; segment is dumped
        if (newLength < 1) {
            isDead = true;
        }
        // Set length ratio for next call to evolve
        lengthRatio = length / newLength;
        // Update length
        length = newLength;

        // Update dependent locs
        findDependentLocs();
    }

    // Sets the ID counter to value
    static void setIDCounter(uint64_t value) {
        global_id_counter.store(value, std::memory_order_seq_cst);
    }

private:
    inline static std::atomic<uint64_t> global_id_counter{0}; // Thread-safe global counter
    static constexpr uint64_t id_cache_size = 4096; // ID cache size

    // Unique segment ID generator (batched and thread-safe)
    static uint64_t nextID() {
        thread_local uint64_t local_idx = 0; // Next ID to use
        thread_local uint64_t local_end = 0; // Marks the end of cached block

        // Refill local cache if needed
        if (local_idx == local_end) {
            // Get next uncached index
            local_idx = global_id_counter.fetch_add(id_cache_size, std::memory_order_relaxed);
            // Set end of new cache
            local_end = local_idx + id_cache_size;
        }

        // Return local_idx, then add one
        return local_idx++;
    }
};

#endif