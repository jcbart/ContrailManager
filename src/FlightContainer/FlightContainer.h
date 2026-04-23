#ifndef FLIGHTCONTAINER_H
#define FLIGHTCONTAINER_H

#include <vector>
#include <string>
#include <unordered_map>
#include <optional>
#include <omp.h>
#include "flight/Flight.h"

// Forward declarations
struct CMTime;

// Struct for storing flight data read from file
struct StagedFlightData {
    std::optional<Aircraft> aircraft;
    std::vector<Waypoint> waypoints;
};

struct FlightContainer {
private:
    // Index of the next flight in loadedFlights to check for adding to activeFlights
    size_t nextFlightToCheck = 0;

    // Map of flight ID to StagedFlightData objects
    // Only used while reading datasets, otherwise empty
    std::unordered_map<std::string, StagedFlightData> flightDataMap;

    // Add waypoint to corresponding ID in flightDataMap
    // If ID does not exist, it is added to the map
    void addWaypointToMap(const std::string& ID, const Waypoint& wp) {
        flightDataMap[ID].waypoints.push_back(wp);
    }

    // Add fixed flight data to corresponding ID in flightDataMap
    // If ID does not exist, it is added to the map
    void addAircraftToMap(const std::string& ID, const Aircraft& aircraft) {
        flightDataMap[ID].aircraft = aircraft;
    }

    // Read waypoint data from paths to flightDataMap
    void readWaypointDatasets(const std::vector<std::string>& filepaths);

    // Read single waypoint file
    void readWaypointFile(const std::string& filepath);
    
    // Read single waypoint file (Parquet)
    void readWaypointParquet(const std::string& filepath);

    // Read aircraft data from paths to flightDataMap
    void readAircraftDatasets(const std::vector<std::string>& filepaths);

    // Read single aircraft file
    void readAircraftFile(const std::string& filepath);

    // Read single aircraft file (Parquet)
    void readAircraftParquet(const std::string& filepath);

public:
    // Flights loaded in memory, sorted by first waypoint time
    std::vector<Flight> loaded;
    // Flights currently in their trajectory
    std::vector<Flight> active;

    float maxInitialSegLen; // Maximum length of a new segment (m)

    // Read waypoint and static datasets from paths
    // Loads all flights into loaded
    void readDatasets(const std::vector<std::string>& waypointFilepaths,
        const std::vector<std::string>& aircraftFilepaths);

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