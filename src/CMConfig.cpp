#include <limits>
#include <yaml-cpp/yaml.h>
#include "CMConfig.h"
#include "CMLog.h"

void CMConfig::read() {
    YAML::Node config = YAML::LoadFile("CM-config.yaml");

    twoWayCoupling = config["Two-way coupling"].as<bool>();

    plumeModelID = config["Plume model"].as<int>();

    restartRun = config["Restart"].as<bool>();

    double outputInterval_min = config["Output interval (min)"].as<double>();
    outputInterval_s = 60 * outputInterval_min;

    maxInitialSegLen = config["Max initial segment length (m)"].as<double>();
    if (maxInitialSegLen <= 0) {
        CM_RaiseError("Config error: Read maximum initial segment length of "
            + std::to_string(maxInitialSegLen)
            + " m. Maximum initial segment length must be positive.", __FILE__, __LINE__);
    }

    double maxContrailAge_h = config["Max contrail age (h)"].as<double>();
    if (maxContrailAge_h <= 0) {
        CM_RaiseError("Config error: Read maximum contrail age of "
            + std::to_string(maxContrailAge_h)
            + " h. Maximum contrail age must be positive.", __FILE__, __LINE__);
    }
    maxContrailAge_s = 3600 * maxContrailAge_h;

    if (twoWayCoupling) {
        // If two-way coupling, read value from config
        maxAccumVapRatio = config["Max accumulated vapour ratio ()"].as<double>();
        if (maxAccumVapRatio <= 0) {
            CM_RaiseError("Config error: Read maximum accumulated vapour ratio of "
                + std::to_string(maxAccumVapRatio)
                + ". Maximum accumulated vapour ratio must be positive.", __FILE__, __LINE__);
        }
    }
    else {
        // Else, value is infinite so that segments are never too massive
        maxAccumVapRatio = std::numeric_limits<double>::infinity();
    }

    flightDatasetPaths = config["Flight dataset paths"].as<std::vector<std::string>>();
}