#ifndef FLIGHTINPUTS_H
#define FLIGHTINPUTS_H

// Flight inputs to contrail segment
struct FlightInputs {
    float engine_efficiency = 0; // Engine efficiency ()
    float ei_h2o = 0; // Emissions index of water vapour (kg (kg fuel)-1)
    float q_fuel = 0; // Specific combustion heat of fuel (J kg-1)
    float aircraft_mass = 0; // Aircraft mass (kg)
    float wingspan = 0; // Aircraft wingspan (m)
    float true_airspeed = 0; // True airspeed (m s-1)
    float fuel_flow = 0; // Fuel flow (kg s-1)
    float T_exhaust = 0; // Exhaust temperature (K)
    float nvpm_ei_n = 0; // Emissions index of nvPM (# (kg fuel)-1)
};

#endif