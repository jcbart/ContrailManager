#include <filesystem>
#include <algorithm>
#include <format>
#include <chrono>
#include "FlightContainer/FlightContainer.h"
#include "timekeeping.h"
#include "CMLog.h"

void FlightContainer::readWaypointFile(const std::string& filepath) {
    CM_LogWrite("Reading waypoint dataset: " + filepath);

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
        readWaypointParquet(filepath);
    }
    else {
        CM_RaiseError("Flight dataset '" + filepath + "' has unsupported file extension.",
            __FILE__, __LINE__);
    }
}

void FlightContainer::readAircraftFile(const std::string& filepath) {
    CM_LogWrite("Reading aircraft dataset: " + filepath);

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
        readAircraftParquet(filepath);
    }
    else {
        CM_RaiseError("Flight dataset '" + filepath + "' has unsupported file extension.",
            __FILE__, __LINE__);
    }
}

void FlightContainer::readWaypointDatasets(const std::vector<std::string>& filepaths) {
    constexpr std::string_view PREFIX_TAG = "PREFIX:";

    for (const std::string& filepath : filepaths) {
        // If a prefix path
        if (filepath.starts_with(PREFIX_TAG)) {
            const std::filesystem::path prefixPath = filepath.substr(PREFIX_TAG.size());

            const std::string filePrefix = prefixPath.filename().string();

            if (filePrefix.empty()) {
                CM_RaiseError(std::format("No prefix provided after {}", PREFIX_TAG),
                    __FILE__, __LINE__);
            }

            // If prefix includes directory path, must search that directory
            const std::filesystem::path dir = prefixPath.has_parent_path()
                ? prefixPath.parent_path()
                : std::filesystem::current_path();

            // Add all matching files in search directory to vector
            std::vector<std::string> matchedFilepaths;
            for (const std::filesystem::directory_entry& entry
                    : std::filesystem::directory_iterator(dir)) {
                if (entry.path().filename().string().starts_with(filePrefix)) {
                    matchedFilepaths.push_back(entry.path().string());
                }
            }

            CM_LogWrite(std::format("Found {} files matching prefix \"{}\"",
                matchedFilepaths.size(), prefixPath.string()));
            
            // Sort by name since this is likely to result in quicker waypoint and flight
            // sorting later
            std::ranges::sort(matchedFilepaths);

            for (const std::string& match : matchedFilepaths) {
                readWaypointFile(match);
            }
        }
        // Else, regular path
        else {
            readWaypointFile(filepath);
        }
    }
}

void FlightContainer::readAircraftDatasets(const std::vector<std::string>& filepaths) {
    constexpr std::string_view PREFIX_TAG = "PREFIX:";

    for (const std::string& filepath : filepaths) {
        // If a prefix path
        if (filepath.starts_with(PREFIX_TAG)) {
            const std::filesystem::path prefixPath = filepath.substr(PREFIX_TAG.size());

            const std::string filePrefix = prefixPath.filename().string();

            if (filePrefix.empty()) {
                CM_RaiseError(std::format("No prefix provided after {}", PREFIX_TAG),
                    __FILE__, __LINE__);
            }

            // If prefix includes directory path, must search that directory
            const std::filesystem::path dir = prefixPath.has_parent_path()
                ? prefixPath.parent_path()
                : std::filesystem::current_path();

            // Add all matching files in search directory to vector
            std::vector<std::string> matchedFilepaths;
            for (const std::filesystem::directory_entry& entry
                    : std::filesystem::directory_iterator(dir)) {
                if (entry.path().filename().string().starts_with(filePrefix)) {
                    matchedFilepaths.push_back(entry.path().string());
                }
            }

            CM_LogWrite(std::format("Found {} files matching prefix \"{}\"",
                matchedFilepaths.size(), prefixPath.string()));

            // Sort by name since this is likely to result in quicker flight sorting later
            std::ranges::sort(matchedFilepaths);

            for (const std::string& match : matchedFilepaths) {
                readAircraftFile(match);
            }
        }
        // Else, regular path
        else {
            readAircraftFile(filepath);
        }
    }
}

void FlightContainer::readDatasets(
    const std::vector<std::string>& waypointFilepaths,
    const std::vector<std::string>& fixedFilepaths
) {
    // Time at start of reading
    std::chrono::steady_clock::time_point readTimeStart = std::chrono::steady_clock::now();

    // The following two methods can be called in either order, but cannot easily be parallelised

    // Read waypoints
    readWaypointDatasets(waypointFilepaths);

    // Read aircraft data
    readAircraftDatasets(fixedFilepaths);

    // Move flights from flightIDMap to loaded
    loaded.reserve(flightDataMap.size());
    for (auto& [ID, stagedData] : flightDataMap) {
        // Ignore staged data with no waypoints
        if (stagedData.waypoints.empty()) {
            continue;
        }

        // If waypoints exist, but no aircraft data, raise error
        if (!stagedData.aircraft.has_value()) {
            CM_RaiseError("No aircraft data found for flight " + ID, __FILE__, __LINE__);
        }

        // Construct Flight in loaded vector
        // Use move to empty the map
        loaded.emplace_back(ID, std::move(*stagedData.aircraft), std::move(stagedData.waypoints));
    }

    // Sort waypoints in each loaded flight by time
    #pragma omp parallel for
    for (Flight& flight : loaded) {
        std::ranges::sort(flight.waypoints, {}, &Waypoint::time);
    }

    // Sort loaded flights by first waypoint time
    std::ranges::sort(loaded, {},
        [](const Flight& flight) {
            return flight.waypoints.front().time;
        }
    );

    // Time at end of reading
    std::chrono::steady_clock::time_point readTimeEnd = std::chrono::steady_clock::now();

    // Elapsed time for reading (seconds)
    std::chrono::duration<double> readTime = readTimeEnd - readTimeStart;

    CM_LogWrite(
        std::format("Loaded {} flights from file in {:.3f} s.", loaded.size(), readTime.count())
    );

    // Warn if no flights have been read
    if (loaded.empty()) {
        CM_LogWarning("No flights read from file. Contrail Manager running idle.");
    }
}

void FlightContainer::updateActive(const CMTime& startTime, const CMTime& stopTime) {
    // Remove flights whose last waypoint is before startTime
    std::erase_if(
        active,
        [startTime](const Flight& flight) {
            return flight.waypoints.back().time <= startTime;
        }
    );

    // Add flights whose first waypoint is between startTime and stopTime
    while (nextFlightToCheck < loaded.size()) {
        const Flight& flight = loaded[nextFlightToCheck];

        // For flights which start before startTime
        // (this path should only be taken the first time the method is called)
        if (flight.waypoints.front().time < startTime) {
            // If it ends after startTime, it is mid-journey; add it
            if (flight.waypoints.back().time >= startTime) {
                active.push_back(flight);
            }
            // Whether it is added or not, move onto next flight
            nextFlightToCheck++;
            continue;
        }

        // If flight starts at or after startTime, but before stopTime, add it
        if (flight.waypoints.front().time < stopTime) {
            active.push_back(flight);
            nextFlightToCheck++;
        }
        // Else, flight starts after window and thus so do all after; end search
        else {
            break;
        }
    }
}