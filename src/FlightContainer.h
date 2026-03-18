#ifndef FLIGHTCONTAINER_H
#define FLIGHTCONTAINER_H

#include <vector>
#include <string>
#include <unordered_map>
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

public:
    // Flights loaded in memory, sorted by first waypoint time
    std::vector<Flight> loaded;
    // Flights currently in their trajectory
    std::vector<Flight> active;

    // Read dataset
    void read_datasets();

    // Read datasets from paths
    void read_datasets(const std::vector<std::string>& filepaths);

    // Update FlightContainer::active by removing flights no longer on their trajectory and adding
    // flights starting their trajectory from FlightContainer::loaded
    void update_active(const CMTime& startTime, const CMTime& stopTime);

private:
    void read_parquet(const std::string& filepath);
};

#endif