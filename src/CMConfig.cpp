#include <yaml-cpp/yaml.h>
#include "CMConfig.h"
#include "CMLog.h"

void CMConfig::read() {
    YAML::Node config = YAML::LoadFile("CM-config.yaml");

    twoWayCoupling = config["Two-way coupling"].as<bool>();

    plumeModelID = config["Plume model"].as<int>();

    maxInitialSegLen = config["Max initial segment length (m)"].as<float>();
    if (maxInitialSegLen <= 0) {
        CM_RaiseError("Config error: Read maximum initial segment length of "
            + std::to_string(maxInitialSegLen)
            + " m. Maximum initial segment length must be positive.", __FILE__, __LINE__);
    }

    float maxContrailAge_h = config["Max contrail age (h)"].as<float>();
    if (maxContrailAge_h <= 0) {
        CM_RaiseError("Config error: Read maximum contrail age of "
            + std::to_string(maxContrailAge_h)
            + " h. Maximum contrail age must be positive.", __FILE__, __LINE__);
    }
    maxContrailAge_s = 3600 * maxContrailAge_h;

    maxAccumVapRatio = config["Max accumulated vapour ratio ()"].as<float>();
    if (maxAccumVapRatio <= 0) {
        CM_RaiseError("Config error: Read maximum accumulated vapour ratio of "
            + std::to_string(maxAccumVapRatio)
            + ". Maximum accumulated vapour ratio must be positive.", __FILE__, __LINE__);
    }

    flightDatasetPaths = config["Flight dataset paths"].as<std::vector<std::string>>();
}