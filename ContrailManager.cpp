#include <iostream>
#include "ContrailManager.h"
#include "variables.h"

void ContrailManager::init() {
    std::cout << "Initialising Contrail Manager:" << std::endl;
    int timeStep_s = 10; // Read from CM config
    // Only accepts hour
    timeStep.set(0, 0, 0, 0, 0, timeStep_s);
    // Read flight data etc
    std::cout << "Contrail Manager initialised" << std::endl;
}

void ContrailManager::run(CMTime& startTime, CMTime& stopTime) {
    // Integrate between times
    currTime = startTime;
    // Use correct addition/comparison
    while (currTime+timeStep <= stopTime) {
        currTime = currTime+timeStep;
        // Do stuff
    }
}

void ContrailManager::init_XLAT(int ids, int ide, int jds, int jde) {
    XLAT.init(ids, ide, jds, jde);
}

void ContrailManager::init_XLONG(int ids, int ide, int jds, int jde) {
    XLONG.init(ids, ide, jds, jde);
}