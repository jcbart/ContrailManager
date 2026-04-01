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

    float outputInterval_s = 3*3600;

    // Maximum length of a new segment (m)
    float maxInitialSegLen = 2500;

    // Maximum age of a contrail segment (s)
    float maxContrailAge_s = 12*3600;

    // Maximum ratio of double-counted water vapour mass in contrail plume to water vapour mass in
    // grid cell ()
    float maxAccumVapRatio = 1e-2;

    std::vector<std::string> flightDatasetPaths; // Flight dataset file paths

    // Read config file
    void read();
};

#endif