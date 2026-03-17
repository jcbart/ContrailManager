#include <filesystem>
#include <algorithm>
#include <format>
#include "FlightContainer.h"
#include "timekeeping.h"
#include "CMLog.h"

void FlightContainer::read_dataset() {
    // Read flight data etc
    Flight test_flight("1");
    CMTime time1 = {2025, 4, 1, 6, 0, 0};
    CMTime time2 = {2025, 4, 1, 6, 3, 0};
    Geo3D loc1 = {-9.7, 52.1, 10500};
    Geo3D loc2 = {-9.1, 52.1, 10500};
    test_flight.waypoints.emplace_back(time1, loc1);
    test_flight.waypoints.emplace_back(time2, loc2);
    loaded.push_back(test_flight);
}

void FlightContainer::read_dataset(const std::string& filepath) {
    // Special case for idle Contrail Manager
    if (filepath == "NONE") {
        CM_LogWarning("Provided flight dataset is 'NONE'. Contrail Manager running idle.");
        return;
    }

    // Check file exists
    if (!std::filesystem::exists(filepath)) {
        CM_RaiseError("File not found: " + filepath, __FILE__, __LINE__);
    }

    // File path
    std::filesystem::path path(filepath);
    // File extension
    std::string extension = path.extension().string();

    // Read file according to its extension
    if (extension == ".parquet" || extension == ".pq") {
        read_parquet(filepath);
    }
    else {
        CM_RaiseError("Flight dataset '" + filepath + "' has unsupported file extension.",
            __FILE__, __LINE__);
    }

    // Sort waypoints in each loaded flight by time
    for (Flight& flight : loaded) {
        std::sort(
            flight.waypoints.begin(),
            flight.waypoints.end(),
            [](const Waypoint& A, const Waypoint& B) {
                return A.time < B.time;
            }
        );
    }

    // Sort loaded flights by first waypoint time
    std::sort(
        loaded.begin(),
        loaded.end(),
        [](const Flight& A, const Flight& B) {
            return A.waypoints[0].time < B.waypoints[0].time;
        }
    );

    CM_LogWrite(std::format("Loaded {} flights from file.", loaded.size()));

    for (const Flight& flight : loaded) {
        CM_LogWrite("Flight ID: " + flight.ID);
        for (const Waypoint& wp : flight.waypoints) {
            CM_LogWrite("Waypoint time: " + wp.time.asString());
        }
    }
}

void FlightContainer::update_active(const CMTime& startTime, const CMTime& stopTime) {
    // Remove flights whose last waypoint is before startTime
    std::erase_if(
        active,
        [startTime](const Flight& flight) {
            return flight.waypoints[flight.numWaypoints() - 1].time <= startTime;
        }
    );

    // Add flights whose first waypoint is between startTime and stopTime
    while (nextFlightToCheck < loaded.size()) {
        // If flight starts before startTime, skip it
        // (this path should only be taken the first time the method is called to skip past
        // flights which start before startTime)
        if (loaded[nextFlightToCheck].waypoints[0].time < startTime) {
            nextFlightToCheck++;
            continue;
        }
        // If flight starts at or after startTime, but before stopTime, add it
        if (loaded[nextFlightToCheck].waypoints[0].time < stopTime) {
            active.push_back(loaded[nextFlightToCheck]);
            nextFlightToCheck++;
        }
        // Else, flight starts after window and thus so do all after; end search
        else {
            break;
        }
    }
}