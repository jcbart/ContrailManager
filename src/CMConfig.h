#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

// Contrail Manager config struct
struct CMConfig {
    // True for two-way coupling (feedback to NWP)
    bool twoWayCoupling = true;

    // ID of the plume model to use
    int plumeModelID = 0;

    // True if the Contrail Manager should initialise from an output file
    bool restartRun = false;

    double outputInterval_s = 3*3600;

    // Maximum length of a new segment (m)
    double maxInitialSegLen = 2e3;

    // Maximum length of a segment at any time (m)
    // Irrelevant (set to infinity) if coupling is one-way so that segments are never too long
    double maxSegLen = 10e3;

    // Maximum age of a contrail segment (s)
    double maxContrailAge_s = 12*3600;

    // Maximum ratio of double-counted water vapour mass in contrail plume to water vapour mass in
    // grid cell ()
    // Irrelevant (set to infinity) if coupling is one-way so that segments are never too massive
    double maxAccumVapRatio = 1e-2;

    std::vector<std::string> flightWaypointPaths; // Flight waypoint file paths

    std::vector<std::string> aircraftDataPaths; // Aircraft data file paths

    // Read config file
    void read();
};

#endif