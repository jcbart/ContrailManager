#ifndef AIRCRAFT_H
#define AIRCRAFT_H

// Aircraft data either read from file or constant
struct Aircraft {
    // Read from file
    float wingspan; // Aircraft wingspan (m)
    
    // Constant for now
    float ei_h2o = 1.25; // Emissions index of water vapour (kg (kg fuel)-1)
    float q_fuel = 43.15e6; // Specific combustion heat of fuel (J kg-1)

    // Empty constructor
    Aircraft() {}

    // Constructor with values
    Aircraft(const float wingspan) : wingspan(wingspan) {}
};

#endif