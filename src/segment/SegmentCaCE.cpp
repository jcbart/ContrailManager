#include <limits>
#include "segment/SegmentCaCE.h"
#include "constants.h"
#include "thermo.h"

// Open unnamed namespace to ensure these functions stay local and do not conflict with CoCiP's
namespace {

// A Newton-Raphson solver for finding the root of function f given its derivative fprime
double newtonRaphsonSolver(std::function<double(double)> f,
                           std::function<double(double)> fprime,
                           double x0, double tol = 1e-7, int maxIter = 100) {
    double x = x0;
    for (int i = 0; i < maxIter; i++) {
        double f_x = f(x);
        double fprime_x = fprime(x);
        if (std::abs(fprime_x) < 1e-12) {
            // Avoid division by 0
            break;
        }
        double x_next = x - f_x / fprime_x;
        if (std::abs(x_next - x) < tol) {
            return x_next;
        }
        x = x_next;
    }
    return x;
}

namespace sac {

    // Calculate the slope of the mixing line in a temperature-humidity diagram (Pa K-1)
    constexpr double slope_mixing_line(double specific_humidity, double air_pressure,
        double engine_efficiency, double ei_h2o, double q_fuel) {
        
        double c_pm = thermo::c_pm(specific_humidity);
        double G = (ei_h2o * c_pm * air_pressure)
                    / (constants::EPS * q_fuel * (1. - engine_efficiency));
        return G;
    }

    // Calculate temperature at which liquid saturation curve has slope G (K)
    constexpr double T_sat_liquid(double G) {
        double log_ = std::log(G - 0.053);
        double T_sat_liquid_ = -46.46 - constants::ABS_ZERO + 9.43*log_ + 0.72*log_*log_;
        return T_sat_liquid_;
    }

    // Calculate critical relative humidity threshold of contrail formation ()
    constexpr double rh_critical_sac(double air_temperature, double T_sat_liq, double G) {
        double e_sat_T_sat_liq = thermo::e_sat_liquid(T_sat_liq);
        double e_sat_T = thermo::e_sat_liquid(air_temperature);
        double rh_crit = (G * (air_temperature - T_sat_liq) + e_sat_T_sat_liq) / e_sat_T;
        rh_crit = std::max(0., std::min(1., rh_crit));
        return (air_temperature > T_sat_liq) ? std::numeric_limits<double>::infinity() : rh_crit;
    }

    // Estimate temperature threshold for persistent contrail formation (K)
    double T_critical_sac(double T_LM, double rh, double G) {
        if (!(rh < 0.999 && std::isfinite(T_LM))) {
            return T_LM;
        }
        double e_L_of_T_LM = thermo::e_sat_liquid(T_LM);

        auto Schumann_eq11 = [T_LM, e_L_of_T_LM, rh, G](double T) -> double {
            return (T - T_LM + (e_L_of_T_LM - rh * thermo::e_sat_liquid(T)) / G);
        };

        auto Schumann_eq11_prime = [rh, G](double T) -> double {
            return (1. - rh * thermo::e_sat_liquid_prime(T) / G);
        };

        double init_guess = T_LM - 1;
        constexpr double tol = 1e-7;
        constexpr int maxIter = 10;
        double T_crit_sac = newtonRaphsonSolver(
            Schumann_eq11, Schumann_eq11_prime, init_guess, tol, maxIter
        );
        return T_crit_sac;
    }

}

namespace wind_shear {

    // Calculate the magnitude of the wind shear (s-1) between two altitudes 
    constexpr double wind_shear(double u_wind_top, double u_wind_btm, double v_wind_top,
        double v_wind_btm, double dz) {
        
        double du_dz = (u_wind_top - u_wind_btm) / dz;
        double dv_dz = (v_wind_top - v_wind_btm) / dz;
        return std::sqrt(du_dz*du_dz + dv_dz*dv_dz);
    }

    // Calculate the wind shear normal to the contrail heading (s-1) between two altitudes 
    constexpr double wind_shear_normal(double u_wind_top, double u_wind_btm, double v_wind_top,
        double v_wind_btm, double cos_a, double sin_a, double dz) {
        
        double du_dz = (u_wind_top - u_wind_btm) / dz;
        double dv_dz = (v_wind_top - v_wind_btm) / dz;
        return (dv_dz * cos_a - du_dz * sin_a);
    }

    // Calculate the multiplication factor to enhance the wind shear based on contrail depth
    // If effective_vertical_resolution (m) is zero, the enhancement is 0.5
    // If wind_shear_enhancement_exponent is zero (and effective_vertical_resolution is not zero),
    // there is no enhancement
    constexpr double wind_shear_enhancement_factor(double contrail_depth,
        double effective_vertical_resolution, double wind_shear_enhancement_exponent) {

        return (
            (contrail_depth > 0) ?
            0.5 * (1 + std::pow(effective_vertical_resolution / contrail_depth,
                                wind_shear_enhancement_exponent))
            : 1
        );
    }

}

namespace wake_vortex {

    // Calculate the effective time scale of the wake vortex (s)
    constexpr double effective_time_scale(double wingspan, double true_airspeed,
        double aircraft_mass, double rho_air) {
        
        return (
            std::pow(constants::PI, 4) / 32 * std::pow(wingspan, 3) * rho_air * true_airspeed
            / (aircraft_mass * constants::GRAVITY)
        );
    }

    // Calculate the maximum contrail downward displacement under strongly stratified
    // conditions (m)
    constexpr double downward_displacement_strongly_stratified(double wingspan,
        double true_airspeed, double aircraft_mass, double rho_air, double n_bv) {
        
        return (
            ((1.49 * 16) / (2 * constants::PI*constants::PI*constants::PI)
            * aircraft_mass * constants::GRAVITY)
            / (wingspan*wingspan * rho_air * true_airspeed * n_bv)
        );
    }

    // Calculate the wake vortex separation (m)
    constexpr double wake_vortex_separation(double wingspan) {
        return (constants::PI * wingspan / 4);
    }

    // Calculate the turbulent kinetic energy dissipation rate (epsilon; m2 s-3)
    // The shear enhancement factor is used to account for any sub-grid scale turbulence
    constexpr double turbulent_kinetic_energy_dissipation_rate(double ds_dz,
        double shear_enhancement_factor) {
        
        return (0.5 * 0.01 * (ds_dz * shear_enhancement_factor*shear_enhancement_factor));
    }

    constexpr double normalized_dissipation_rate(double epsilon, double wingspan,
        double true_airspeed, double aircraft_mass, double rho_air) {
        
        double c = std::pow(constants::PI/4, 1./3.) * constants::PI*constants::PI*constants::PI / 8;
        double numer = c * std::pow(epsilon * wingspan, 1./3.) * wingspan*wingspan * rho_air
            * true_airspeed;
        double epsn_st = numer / (constants::GRAVITY * aircraft_mass);
        return std::min(epsn_st, 0.36);
    }

    double downward_displacement_weakly_stratified(double wingspan, double true_airspeed,
        double aircraft_mass, double rho_air, double n_bv, double dz_max_strong, double ds_dz,
        double t_0, double effective_vertical_resolution, double wind_shear_enhancement_exponent) {
        
        double b_0 = wake_vortex_separation(wingspan);
        double dz_max = std::max(dz_max_strong, 10.);
        double shear_enhancement_factor = wind_shear::wind_shear_enhancement_factor(dz_max,
            effective_vertical_resolution, wind_shear_enhancement_exponent);
        double epsn = turbulent_kinetic_energy_dissipation_rate(ds_dz, shear_enhancement_factor);
        double epsn_st = normalized_dissipation_rate(epsn, wingspan, true_airspeed, aircraft_mass,
            rho_air);
        return (b_0 * (
            7.68 * (1 - 4.07 * epsn_st + 5.67 * epsn_st*epsn_st) * (0.79 - n_bv * t_0) + 1.88
        ));
    }

    double max_downward_displacement(double wingspan, double true_airspeed,
        double aircraft_mass, double air_temperature, double dtheta_dz, double ds_dz,
        double air_pressure, double effective_vertical_resolution,
        double wind_shear_enhancement_exponent) {
        
        double rho_air = thermo::rho_d(air_temperature, air_pressure);
        double n_bv = thermo::brunt_vaisala_frequency(air_pressure, air_temperature, dtheta_dz);
        double t_0 = effective_time_scale(wingspan, true_airspeed, aircraft_mass, rho_air);
        bool is_weakly_stratified = (n_bv * t_0 < 0.8);
        double dz_max_strong = downward_displacement_strongly_stratified(wingspan, true_airspeed,
            aircraft_mass, rho_air, n_bv);
        double dz_max;
        if (is_weakly_stratified) {
            dz_max = downward_displacement_weakly_stratified(wingspan, true_airspeed,
                aircraft_mass, rho_air, n_bv, dz_max_strong, ds_dz, t_0, effective_vertical_resolution,
                wind_shear_enhancement_exponent);
        }
        else {
            dz_max = dz_max_strong;
        }
        return dz_max;    
    }

    // Calculate the initial contrail width (m)
    constexpr double initial_contrail_width(double wingspan) {
        return (constants::PI/4 * wingspan);
    }

    // Calculate the initial contrail depth (m)
    constexpr double initial_contrail_depth(double dz_max, double initial_wake_vortex_depth) {
        return (dz_max * initial_wake_vortex_depth);
    }

}

namespace contrail_properties {

    // Calculate the specific humidity released by water vapor from aircraft emissions
    constexpr double q_exhaust(double air_temperature, double air_pressure,
        double fuel_dist, double width, double depth, double ei_h2o) {
        
        return (
            (ei_h2o * fuel_dist)
            / ((constants::PI / 4) * width * depth * thermo::rho_d(air_temperature, air_pressure))
        );
    }

    // Estimate the initial contrail ice water content (iwc; kg (kg air)-1) before the wake vortex
    // phase
    constexpr double initial_iwc(double air_temperature, double specific_humidity,
        double air_pressure, double fuel_dist, double width, double depth, double ei_h2o) {
        
        double q_sat = thermo::q_sat_ice(air_temperature, air_pressure);
        double q_exh = q_exhaust(air_temperature, air_pressure, fuel_dist, width, depth, ei_h2o);
        return std::max(q_exh + specific_humidity - q_sat, 0.);
    }

    // Calculate the ambient air temperature (K) after the wake vortex phase
    constexpr double temperature_adiabatic_heating(double air_temperature_pre_vortex,
        double air_pressure_pre_vortex, double air_pressure_post_vortex) {
        
        return air_temperature_pre_vortex * std::pow(
            (air_pressure_post_vortex / air_pressure_pre_vortex),
            (constants::GAMMA - 1) / constants::GAMMA
        );
    }

    // Calculate the change in ice water content (kg (kg air)-1) due to adiabatic heating from the
    // wake vortex phase
    double iwc_adiabatic_heating(double air_temperature_pre_vortex,
        double air_pressure_pre_vortex, double air_pressure_post_vortex) {
        
        double e_sat_ice_pre_vortex = thermo::e_sat_ice(air_temperature_pre_vortex);
        double air_temperature_post_vortex = temperature_adiabatic_heating(
            air_temperature_pre_vortex, air_pressure_pre_vortex, air_pressure_post_vortex);
        
        double e_sat_ice_post_vortex = thermo::e_sat_ice(air_temperature_post_vortex);

        double delta_q = (constants::EPS) * (
            (e_sat_ice_post_vortex / air_pressure_post_vortex) 
            - (e_sat_ice_pre_vortex / air_pressure_pre_vortex)
        );
        return std::max(delta_q, 0.);
    }

    // Calculate the ice water content after the wake vortex phase
    constexpr double iwc_post_wake_vortex(double iwc, double iwc_ad) {
        return std::max(iwc - iwc_ad, 0.);
    }

    // Calculate the initial number of ice particles per distance after the wake vortex phase
    constexpr double initial_ice_particle_number(double aei, double fuel_dist,
        double min_aei) {
        
        return fuel_dist * std::max(aei, min_aei);
    }

    // Calculate the activation rate of black carbon particles to contrail ice crystals
    // No vPM contribution
    constexpr double ice_particle_activation_rate(double air_temperature,
        double T_crit_sac) {
        
        // Ignore rounding line
        return (-0.661 * std::exp(std::min(air_temperature - T_crit_sac, 0.)) + 1);
    }

    // Calculate effective cross-sectional area of contrail plume from sigma parameters (m2)
    // This function calculates the same output as plume_effective_cross_sectional_area, but
    // calculated with different input parameters
    constexpr double new_effective_area_from_sigma(double sigma_yy,
        double sigma_zz, double sigma_yz) {
        
        return (2 * constants::PI * std::sqrt(sigma_yy * sigma_zz - sigma_yz*sigma_yz));
    }

    // Calculate the effective cross-sectional area of the contrail plume (m2)
    constexpr double plume_effective_cross_sectional_area(double width, double depth,
        double sigma_yz) {
        
        double sigma_yy = 0.125 * width*width;
        double sigma_zz = 0.125 * depth*depth;
        return new_effective_area_from_sigma(sigma_yy, sigma_zz, sigma_yz);
    }

}

namespace unterstrasser_wake_vortex {

    // Calculate area of the wake-vortex plume (m2)
    constexpr double plume_area(double wingspan) {
        double r_plume = 1.5 + 0.314 * wingspan;
        return (2 * constants::PI * r_plume*r_plume);
    }

    // Calculate the total length-scale effect of the wake vortex downwash (m)
    constexpr double z_total_length_scale(double z_atm, double z_emit, double z_desc,
        double true_airspeed, double fuel_flow, double aei_n, double wingspan) {
        
        double fuel_dist = fuel_flow / true_airspeed;
        double n_ice_dist = fuel_dist * aei_n;
        double n_ice_per_vol = n_ice_dist / plume_area(wingspan);
        double n_ice_per_vol_ref = 3.38e12 / plume_area(60.3);
        double psi = std::pow(n_ice_per_vol_ref / n_ice_per_vol, 0.16);
        return (psi * (1.27 * z_atm + 0.42 * z_emit) - 0.49 * z_desc);
    }

    // Calculate the length-scale effect of ambient supersaturation on the ice crystal mass
    // budget (m)
    constexpr double z_atm_length_scale_analytical(double air_temperature, double rh_i) {
        
        // Only perform operation when the ambient condition is supersaturated w.r.t. ice
        // Otherwise, z_atm = 0
        double s_i_clamped = std::max(rh_i - 1, 0.);
        double z_atm = 607.46 * std::pow(s_i_clamped, 0.897)
            * std::pow(air_temperature/205, 2.225);
        return z_atm;
    }

    // Calculate aircraft-emitted water vapour concentration in the plume (kg m-3)
    constexpr double emitted_water_vapour_concentration(double ei_h2o, double wingspan,
        double true_airspeed, double fuel_flow) {
        
        double h2o_per_dist = (ei_h2o * fuel_flow) / true_airspeed;
        double area_p = plume_area(wingspan);
        return (h2o_per_dist / area_p);
    }

    // Calculate the length-scale effect of water vapour emissions on the ice crystal mass
    // budget (m)
    constexpr double z_emit_length_scale_analytical(double rho_emit, double air_temperature) {
        
        double t_205 = air_temperature - 205;
        double z_emit = 1106.6 * std::pow(rho_emit * 1e5, 0.678 + 0.0116 * t_205)
                    * std::exp(-(0.0807 + 0.000428 * t_205) * t_205);
        return z_emit;
    }

    // Calculate fraction of ice particle number surviving the wake vortex phase
    constexpr double survival_fraction_from_length_scale(double z_total) {
        double f_surv = 0.42 + (1.31 / constants::PI) * std::atan(-1 + z_total/100);
        f_surv = std::max(0., std::min(1., f_surv));
        return f_surv;
    }

    // Calculate fraction of ice particle number surviving the wake vortex phase and required
    // inputs; based on Unterstrasser et al. (2016)
    double ice_particle_number_survival_fraction(double air_temperature, double rh_i,
        double ei_h2o, double wingspan, double true_airspeed, double fuel_flow,
        double aei_n, double z_desc) {
        
        double rho_emit = emitted_water_vapour_concentration(ei_h2o, wingspan, true_airspeed,
            fuel_flow);

        // Analytical
        double z_atm = z_atm_length_scale_analytical(air_temperature, rh_i);
        double z_emit = z_emit_length_scale_analytical(rho_emit, air_temperature);

        double z_total = z_total_length_scale(z_atm, z_emit, z_desc, true_airspeed, fuel_flow,
            aei_n, wingspan);
        return survival_fraction_from_length_scale(z_total);
    }

}

} // Close unnamed namespace

void SegmentCaCE::update_met() {
    // Find index of grid cell centre below altitude
    IDX<3, int> ijk = domain->loc_to_ijk(centre);
    int k_below;
    if (!domain->find_k_below(centre, ijk, k_below)) {
        isDead = true;
        return;
    }
    IDX<3, int> ijkBelow = ijk;
    IDX<3, int> ijkAbove = ijk;
    ijkBelow[2] = k_below;
    ijkAbove[2] = k_below + 1;

    // Find height fraction of altitude between grid cell centres
    double interpFraction = calcInterpFraction(centre.alt, ijkBelow, ijkAbove);

    // Interpolate values
    air_pressure = interp_P(ijkBelow, ijkAbove, interpFraction);
    air_temperature = thermo::theta_to_T(
        interp_T_POT(ijkBelow, ijkAbove, interpFraction), air_pressure
    );
    specific_humidity = thermo::r_to_q(interp_QV(ijkBelow, ijkAbove, interpFraction));
    u_wind = interp_U(ijkBelow, ijkAbove, interpFraction);
    v_wind = interp_V(ijkBelow, ijkAbove, interpFraction);

    // Find index of grid cell centre dz_m below altitude
    Geo3D centreLower = centre;
    centreLower.alt -= dz_m;
    IDX<3, int> ijkLower;
    if (!domain->loc_to_ijk(centreLower, ijkLower)) {
        isDead = true;
        return;
    }
    int k_below_lower;
    if (!domain->find_k_below(centreLower, ijkLower, k_below_lower)) {
        isDead = true;
        return;
    }
    IDX<3, int> ijkBelowLower = ijkLower;
    IDX<3, int> ijkAboveLower = ijkLower;
    ijkBelowLower[2] = k_below_lower;
    ijkAboveLower[2] = k_below_lower + 1;

    // Find height fraction of altitude between grid cell centres
    double interpFractionLower = calcInterpFraction(centre.alt - dz_m, ijkBelowLower, ijkAboveLower);

    // Interpolate lower values
    air_pressure_lower = interp_P(ijkBelowLower, ijkAboveLower, interpFractionLower);
    air_temperature_lower = thermo::theta_to_T(
        interp_T_POT(ijkBelowLower, ijkAboveLower, interpFractionLower), air_pressure_lower
    );
    u_wind_lower = interp_U(ijkBelowLower, ijkAboveLower, interpFractionLower);
    v_wind_lower = interp_V(ijkBelowLower, ijkAboveLower, interpFractionLower);

    effective_vertical_resolution = domain->Z.get(ijkAbove) - domain->Z.get(ijkBelow);

    rho_air = thermo::rho_d(air_temperature, air_pressure);

    dtheta_dz = thermo::theta_gradient(air_temperature, air_pressure,
        air_temperature_lower, air_pressure_lower, dz_m);

    ds_dz = wind_shear::wind_shear(u_wind, u_wind_lower, v_wind, v_wind_lower, dz_m);
    
    dsn_dz = wind_shear::wind_shear_normal(u_wind, u_wind_lower, v_wind, v_wind_lower,
        std::cos(90. - heading), std::sin(90. - heading), dz_m);
}

void SegmentCaCE::simulate_wake_vortex_downwash() {
    double dz_max = wake_vortex::max_downward_displacement(wingspan, true_airspeed, aircraft_mass,
        air_temperature, dtheta_dz, ds_dz, air_pressure,
        effective_vertical_resolution, wind_shear_enhancement_exponent);

    width = wake_vortex::initial_contrail_width(wingspan);
    depth = wake_vortex::initial_contrail_depth(dz_max, initial_wake_vortex_depth);
}

void SegmentCaCE::initial_properties() {
    float fuel_dist = fuel_flow / true_airspeed; // (kg m-1)
    centre.alt -= 0.5 * depth;
    front.alt -= 0.5 * depth;
    back.alt -= 0.5 * depth;

    // Save pre-vortex values required below since pre- and post- vortex are both used
    float air_pressure_pre_vortex = air_pressure;
    float air_temperature_pre_vortex = air_temperature;
    float specific_humidity_pre_vortex = specific_humidity;
    // Update local meteorology after change in altitude
    update_met();

    // Uses pre-vortex values
    double iwc_pre_vortex = contrail_properties::initial_iwc(air_temperature_pre_vortex,
        specific_humidity_pre_vortex, air_pressure_pre_vortex, fuel_dist, width, depth, ei_h2o);

    double iwc_ad = contrail_properties::iwc_adiabatic_heating(air_temperature_pre_vortex,
        air_pressure_pre_vortex, air_pressure);

    iwc = contrail_properties::iwc_post_wake_vortex(iwc_pre_vortex, iwc_ad);

    // Uses air temperature pre-vortex
    double f_activation = contrail_properties::ice_particle_activation_rate(
        air_temperature_pre_vortex, T_crit_sac);
    double aei = nvpm_ei_n * f_activation;

    double n_ice_per_m_pre_vortex = contrail_properties::initial_ice_particle_number(aei,
        fuel_dist, min_ice_particle_number_nvpm_ei_n);

    // Unterstrasser version
    double rh_i_pre_vortex = specific_humidity_pre_vortex
        / thermo::q_sat_ice(air_temperature_pre_vortex, air_pressure_pre_vortex);
    double f_surv = unterstrasser_wake_vortex::ice_particle_number_survival_fraction(
        air_temperature_pre_vortex, rh_i_pre_vortex, ei_h2o, wingspan, true_airspeed, fuel_flow,
        nvpm_ei_n, 0.5 * depth);

    n_ice_per_m = n_ice_per_m_pre_vortex * f_surv;

    // Use sigma_yz = 0
    area_eff = contrail_properties::plume_effective_cross_sectional_area(width, depth, 0);
}

void SegmentCaCE::formation() {
    update_met();

    double fuel_dist = fuel_flow / true_airspeed; // (kg fuel m-1)
    double vap_mass_per_m_exhaust = ei_h2o * fuel_dist; // (kg vapour m-1)
    // Mass of vapour exhausted (kg) - used in both formation and no formation
    double M_v_exhaust = vap_mass_per_m_exhaust * length;

    if (!std::isfinite(M_v_exhaust)) {
        badSimulation = true;
        return;
    }

    IDX<3, int> ijk = domain->loc_to_ijk(centre);

    float specific_humidity = thermo::r_to_q(domain->QV.get(ijk));
    float air_pressure = domain->P.get(ijk);
    float air_temperature = thermo::theta_to_T(domain->T_POT.get(ijk), air_pressure);

    double G = sac::slope_mixing_line(specific_humidity, air_pressure, engine_efficiency,
        ei_h2o, q_fuel);
    double T_sat_liq = sac::T_sat_liquid(G);
    double rh_crit_sac = sac::rh_critical_sac(air_temperature, T_sat_liq, G);
    double rh = specific_humidity / thermo::q_sat_liquid(air_temperature, air_pressure);
    T_crit_sac = sac::T_critical_sac(T_sat_liq, rh, G);

    if (!(std::isfinite(rh) && std::isfinite(rh_crit_sac) && (rh > rh_crit_sac))) {
        noFormation = true;
        // If two-way coupling, return water vapour to atmosphere
        if (domain->twoWayCoupling) {
            // Get dry mass of grid cell contrail is inside
            double gridDryMass = domain->DRYMASS.get(ijk);

            // Add M_v_exhaust to current grid cell
            domain->delta_QV.add(ijk,  M_v_exhaust / gridDryMass);
        }
        return;
    }

    simulate_wake_vortex_downwash();
    initial_properties();

    double M_ice = totalIceMass();

    // Specific humidity inside contrail
    double q_sat = thermo::q_sat_ice(air_temperature, air_pressure);
    double M_v_inside = thermo::q_to_r(q_sat) * rho_air * area_eff * length;

    // Check resulting variables are valid
    if (!std::isfinite(iwc) || !std::isfinite(n_ice_per_m) || !std::isfinite(M_v_inside)
        || (M_v_inside > 1e20)) {
        
        badSimulation = true;
        return;
    }

    // Vapour mass intaken from atmosphere =
    //    ice mass + vapour mass inside - vapour mass exhausted
    M_v_accum = M_ice + M_v_inside - M_v_exhaust;

    // Segment must be dumped at the end of this coupling interval
    isDead = true;
}

void SegmentCaCE::dump() {
    IDX<3, int> ijk = domain->loc_to_ijk(centre);

    double gridDryMass = domain->DRYMASS.get(ijk);

    // Specific humidity inside contrail
    double q_sat = thermo::q_sat_ice(air_temperature, air_pressure);
    // Water vapour mass (returned is mass inside minus that double-counted from atmosphere)
    double M_v = thermo::q_to_r(q_sat) * rho_air * area_eff * length;
    domain->delta_QV.add(ijk, (M_v - M_v_accum) / gridDryMass);

    // Ice mass
    double M_ice = totalIceMass();
    domain->delta_QI.add(ijk, M_ice / gridDryMass);

    // Ice number
    double N_ice = totalIceNumber();
    domain->delta_NI.add(ijk, N_ice / gridDryMass);
}