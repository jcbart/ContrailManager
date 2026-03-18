#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

// Contrail Manager config struct
struct CMConfig {
    int plumeModelID = 0; // ID of the plume model to use
    bool twoWayCoupling = true; // True for two-way coupling (feedback to NWP)
    float maxInitialSegLen = 2500; // Maximum length of a new segment (m)
    float maxContrailAge_s = 12*3600; // Maximum age of a contrail segment (s)
    // Maximum ratio of double-counted water vapour mass in contrail plume to water vapour mass in
    // grid cell ()
    float maxAccumVapRatio = 1e-2;

    std::vector<std::string> flightDatasetPaths; // Flight dataset file paths

    // Read config file
    void read();
};

#endif