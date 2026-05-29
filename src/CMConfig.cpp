#include <limits>
#include <format>
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
        CM_RaiseError(std::format("Config error: Read maximum initial segment length of {} m. "
            "Maximum initial segment length must be positive.", maxInitialSegLen),
            __FILE__, __LINE__);
    }

    if (twoWayCoupling) {
        // If two-way coupling, read value from config
        maxSegLen = config["Max segment length (m)"].as<double>();
        if (maxSegLen <= 0) {
            CM_RaiseError(std::format("Config error: Read maximum segment length of {} m. "
                "Maximum segment length must be positive.", maxSegLen),
                __FILE__, __LINE__);
        }
    }
    else {
        // Else, value is infinite so that segments are never too long
        maxSegLen = std::numeric_limits<double>::infinity();
    }

    double maxSegmentAge_h = config["Max segment age (h)"].as<double>();
    if (maxSegmentAge_h <= 0) {
        CM_RaiseError(std::format("Config error: Read maximum segment age of {} h. "
            "Maximum segment age must be positive.", maxSegmentAge_h),
            __FILE__, __LINE__);
    }
    maxSegmentAge_s = 3600 * maxSegmentAge_h;

    if (twoWayCoupling) {
        // If two-way coupling, read value from config
        maxAccumVapRatio = config["Max accumulated vapour ratio ()"].as<double>();
        if (maxAccumVapRatio <= 0) {
            CM_RaiseError(std::format("Config error: Read maximum accumulated vapour ratio of {}. "
                "Maximum accumulated vapour ratio must be positive.", maxAccumVapRatio),
                __FILE__, __LINE__);
        }
    }
    else {
        // Else, value is infinite so that segments are never too massive
        maxAccumVapRatio = std::numeric_limits<double>::infinity();
    }

    flightWaypointPaths = config["Flight waypoint paths"].as<std::vector<std::string>>();

    aircraftDataPaths = config["Aircraft data paths"].as<std::vector<std::string>>();
}