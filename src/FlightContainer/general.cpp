#include <filesystem>
#include <algorithm>
#include <format>
#include <chrono>
#include <omp.h>
#include "FlightContainer.h"
#include "timekeeping.h"
#include "CMLog.h"

void FlightContainer::readFile(const std::string& filepath) {
    CM_LogWrite("Reading flight dataset: " + filepath);

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
        readParquet(filepath);
    }
    else {
        CM_RaiseError("Flight dataset '" + filepath + "' has unsupported file extension.",
            __FILE__, __LINE__);
    }
}

void FlightContainer::readDatasets(const std::vector<std::string>& filepaths) {
    // Time at start of reading
    std::chrono::steady_clock::time_point readTimeStart = std::chrono::steady_clock::now();

    constexpr std::string_view PREFIX_TAG = "PREFIX:";

    for (const std::string& filepath : filepaths) {
        // Ignore NONE used for idle Contrail Manager
        if (filepath == "NONE") {
            return;
        }

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
                readFile(match);
            }
        }
        // Else, regular path
        else {
            readFile(filepath);
        }
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
        // If flight starts before startTime, skip it
        // (this path should only be taken the first time the method is called to skip past
        // flights which start before startTime)
        if (loaded[nextFlightToCheck].waypoints.front().time < startTime) {
            nextFlightToCheck++;
            continue;
        }
        // If flight starts at or after startTime, but before stopTime, add it
        if (loaded[nextFlightToCheck].waypoints.front().time < stopTime) {
            active.push_back(loaded[nextFlightToCheck]);
            nextFlightToCheck++;
        }
        // Else, flight starts after window and thus so do all after; end search
        else {
            break;
        }
    }
}