#ifndef FLIGHTCONTAINER_H
#define FLIGHTCONTAINER_H

#include <vector>
#include <string>
#include <unordered_map>
#include <omp.h>
#include "Flight.h"

// Forward declarations
struct CMTime;

struct FlightContainer {
private:
    // Index of the next flight in loadedFlights to check for adding to activeFlights
    size_t nextFlightToCheck = 0;

    // Map of flight ID to index in FlightContainer::loaded
    // Only valid (and used) during dataset reading
    std::unordered_map<std::string, size_t> flightIDIndexMap;

    void readFile(const std::string& filepath);

    void readParquet(const std::string& filepath);

public:
    // Flights loaded in memory, sorted by first waypoint time
    std::vector<Flight> loaded;
    // Flights currently in their trajectory
    std::vector<Flight> active;

    float maxInitialSegLen; // Maximum length of a new segment (m)

    // Read datasets from paths
    void readDatasets(const std::vector<std::string>& filepaths);

    // Update FlightContainer::active by removing flights no longer on their trajectory and adding
    // flights starting their trajectory from FlightContainer::loaded
    void updateActive(const CMTime& startTime, const CMTime& stopTime);

    // Create segments from each flight (parallelised)
    // For each segment, a flight passes the resulting FlightInputs to `emit`
    template <typename Emit>
    void createSegments(const CMTime& startTime, const CMTime& stopTime, const Domain& domain,
        Emit&& emit) {
        
        #pragma omp parallel for schedule(guided)
        for (const Flight& flight : active) {
            flight.createSegments(startTime, stopTime, domain, maxInitialSegLen, emit);
        }
    }
};

#endif