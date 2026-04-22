#include <iostream>
#include <fstream>
#include <string>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include "PlumeModels.h"
#include "serialization/SerializeSegment.h"
#ifdef WITH_COCIP
#include "serialization/SerializeCoCiP.h"
#endif
#include "readbin.h"

#ifdef WITH_COCIP
arrow::Result<std::shared_ptr<arrow::Table>> CoCiP_to_table(const std::vector<SegmentCoCiP>& vec) {
    arrow::UInt64Builder ID_builder;
    arrow::StringBuilder parentID_builder;
    auto timestamp_type = arrow::timestamp(arrow::TimeUnit::MILLI);
    arrow::TimestampBuilder birthTime_builder(
        timestamp_type, arrow::default_memory_pool()
    );
    arrow::DoubleBuilder centrelon_builder;
    arrow::DoubleBuilder centrelat_builder;
    arrow::DoubleBuilder centrealt_builder;
    arrow::DoubleBuilder heading_builder;
    arrow::DoubleBuilder length_builder;
    arrow::DoubleBuilder width_builder;
    arrow::DoubleBuilder depth_builder;
    arrow::DoubleBuilder area_eff_builder;
    arrow::DoubleBuilder sigma_yz_builder;
    arrow::DoubleBuilder n_ice_per_m_builder;
    arrow::DoubleBuilder n_ice_per_vol_builder;
    arrow::DoubleBuilder iwc_builder;
    arrow::DoubleBuilder plume_mass_per_m_builder;
    arrow::DoubleBuilder r_ice_vol_builder;
    arrow::DoubleBuilder tau_contrail_builder;
    arrow::DoubleBuilder terminal_fall_speed_builder;
    arrow::DoubleBuilder diffuse_h_builder;
    arrow::DoubleBuilder diffuse_v_builder;
    arrow::DoubleBuilder dn_dt_agg_builder;
    arrow::DoubleBuilder dn_dt_turb_builder;
    arrow::DoubleBuilder heat_rate_builder;
    arrow::DoubleBuilder d_heat_rate_builder;
    arrow::DoubleBuilder rf_sw_builder;
    arrow::DoubleBuilder rf_lw_builder;
    arrow::DoubleBuilder rf_net_builder;
    arrow::DoubleBuilder cumul_heat_builder;
    arrow::DoubleBuilder cumul_differential_heat_builder;

    for (const SegmentCoCiP& seg : vec) {
        ARROW_RETURN_NOT_OK(ID_builder.Append(seg.ID));
        ARROW_RETURN_NOT_OK(parentID_builder.Append(seg.parentID));
        int64_t ms = seg.birthTime.timepoint.time_since_epoch().count();
        ARROW_RETURN_NOT_OK(birthTime_builder.Append(ms));
        ARROW_RETURN_NOT_OK(centrelon_builder.Append(seg.centre.lon));
        ARROW_RETURN_NOT_OK(centrelat_builder.Append(seg.centre.lat));
        ARROW_RETURN_NOT_OK(centrealt_builder.Append(seg.centre.alt));
        ARROW_RETURN_NOT_OK(heading_builder.Append(seg.heading));
        ARROW_RETURN_NOT_OK(length_builder.Append(seg.length));
        ARROW_RETURN_NOT_OK(width_builder.Append(seg.cocip.width));
        ARROW_RETURN_NOT_OK(depth_builder.Append(seg.cocip.depth));
        ARROW_RETURN_NOT_OK(area_eff_builder.Append(seg.cocip.area_eff));
        ARROW_RETURN_NOT_OK(sigma_yz_builder.Append(seg.cocip.sigma_yz));
        ARROW_RETURN_NOT_OK(n_ice_per_m_builder.Append(seg.cocip.n_ice_per_m));
        ARROW_RETURN_NOT_OK(n_ice_per_vol_builder.Append(seg.cocip.n_ice_per_vol));
        ARROW_RETURN_NOT_OK(iwc_builder.Append(seg.cocip.iwc));
        ARROW_RETURN_NOT_OK(plume_mass_per_m_builder.Append(seg.cocip.plume_mass_per_m));
        ARROW_RETURN_NOT_OK(r_ice_vol_builder.Append(seg.cocip.r_ice_vol));
        ARROW_RETURN_NOT_OK(tau_contrail_builder.Append(seg.cocip.tau_contrail));
        ARROW_RETURN_NOT_OK(terminal_fall_speed_builder.Append(seg.cocip.terminal_fall_speed));
        ARROW_RETURN_NOT_OK(diffuse_h_builder.Append(seg.cocip.diffuse_h));
        ARROW_RETURN_NOT_OK(diffuse_v_builder.Append(seg.cocip.diffuse_v));
        ARROW_RETURN_NOT_OK(dn_dt_agg_builder.Append(seg.cocip.dn_dt_agg));
        ARROW_RETURN_NOT_OK(dn_dt_turb_builder.Append(seg.cocip.dn_dt_turb));
        ARROW_RETURN_NOT_OK(heat_rate_builder.Append(seg.cocip.heat_rate));
        ARROW_RETURN_NOT_OK(d_heat_rate_builder.Append(seg.cocip.d_heat_rate));
        ARROW_RETURN_NOT_OK(rf_sw_builder.Append(seg.cocip.rf_sw));
        ARROW_RETURN_NOT_OK(rf_lw_builder.Append(seg.cocip.rf_lw));
        ARROW_RETURN_NOT_OK(rf_net_builder.Append(seg.cocip.rf_net));
        ARROW_RETURN_NOT_OK(cumul_heat_builder.Append(seg.cocip.cumul_heat));
        ARROW_RETURN_NOT_OK(cumul_differential_heat_builder.Append(seg.cocip.cumul_differential_heat));
    }

    std::shared_ptr<arrow::Array> ID, parentID, birthTime, centrelon, centrelat, centrealt,
        heading, length, width, depth, area_eff, sigma_yz, n_ice_per_m, n_ice_per_vol, iwc,
        plume_mass_per_m, r_ice_vol, tau_contrail, terminal_fall_speed, diffuse_h, diffuse_v,
        dn_dt_agg, dn_dt_turb, heat_rate, d_heat_rate, rf_sw, rf_lw, rf_net, cumul_heat,
        cumul_differential_heat;

    ARROW_RETURN_NOT_OK(ID_builder.Finish(&ID));
    ARROW_RETURN_NOT_OK(parentID_builder.Finish(&parentID));
    ARROW_RETURN_NOT_OK(birthTime_builder.Finish(&birthTime));
    ARROW_RETURN_NOT_OK(centrelon_builder.Finish(&centrelon));
    ARROW_RETURN_NOT_OK(centrelat_builder.Finish(&centrelat));
    ARROW_RETURN_NOT_OK(centrealt_builder.Finish(&centrealt));
    ARROW_RETURN_NOT_OK(heading_builder.Finish(&heading));
    ARROW_RETURN_NOT_OK(length_builder.Finish(&length));
    ARROW_RETURN_NOT_OK(width_builder.Finish(&width));
    ARROW_RETURN_NOT_OK(depth_builder.Finish(&depth));
    ARROW_RETURN_NOT_OK(area_eff_builder.Finish(&area_eff));
    ARROW_RETURN_NOT_OK(sigma_yz_builder.Finish(&sigma_yz));
    ARROW_RETURN_NOT_OK(n_ice_per_m_builder.Finish(&n_ice_per_m));
    ARROW_RETURN_NOT_OK(n_ice_per_vol_builder.Finish(&n_ice_per_vol));
    ARROW_RETURN_NOT_OK(iwc_builder.Finish(&iwc));
    ARROW_RETURN_NOT_OK(plume_mass_per_m_builder.Finish(&plume_mass_per_m));
    ARROW_RETURN_NOT_OK(r_ice_vol_builder.Finish(&r_ice_vol));
    ARROW_RETURN_NOT_OK(tau_contrail_builder.Finish(&tau_contrail));
    ARROW_RETURN_NOT_OK(terminal_fall_speed_builder.Finish(&terminal_fall_speed));
    ARROW_RETURN_NOT_OK(diffuse_h_builder.Finish(&diffuse_h));
    ARROW_RETURN_NOT_OK(diffuse_v_builder.Finish(&diffuse_v));
    ARROW_RETURN_NOT_OK(dn_dt_agg_builder.Finish(&dn_dt_agg));
    ARROW_RETURN_NOT_OK(dn_dt_turb_builder.Finish(&dn_dt_turb));
    ARROW_RETURN_NOT_OK(heat_rate_builder.Finish(&heat_rate));
    ARROW_RETURN_NOT_OK(d_heat_rate_builder.Finish(&d_heat_rate));
    ARROW_RETURN_NOT_OK(rf_sw_builder.Finish(&rf_sw));
    ARROW_RETURN_NOT_OK(rf_lw_builder.Finish(&rf_lw));
    ARROW_RETURN_NOT_OK(rf_net_builder.Finish(&rf_net));
    ARROW_RETURN_NOT_OK(cumul_heat_builder.Finish(&cumul_heat));
    ARROW_RETURN_NOT_OK(cumul_differential_heat_builder.Finish(&cumul_differential_heat));

    std::shared_ptr<arrow::Schema> schema = arrow::schema({
        arrow::field("ID", arrow::uint64()),
        arrow::field("parent ID", arrow::utf8()),
        arrow::field("birth time", timestamp_type),
        arrow::field("centre lon", arrow::float64()),
        arrow::field("centre lat", arrow::float64()),
        arrow::field("centre alt", arrow::float64()),
        arrow::field("heading", arrow::float64()),
        arrow::field("length", arrow::float64()),
        arrow::field("width", arrow::float64()),
        arrow::field("depth", arrow::float64()),
        arrow::field("area_eff", arrow::float64()),
        arrow::field("sigma_yz", arrow::float64()),
        arrow::field("n_ice_per_m", arrow::float64()),
        arrow::field("n_ice_per_vol", arrow::float64()),
        arrow::field("iwc", arrow::float64()),
        arrow::field("plume_mass_per_m", arrow::float64()),
        arrow::field("r_ice_vol", arrow::float64()),
        arrow::field("tau_contrail", arrow::float64()),
        arrow::field("terminal_fall_speed", arrow::float64()),
        arrow::field("diffuse_h", arrow::float64()),
        arrow::field("diffuse_v", arrow::float64()),
        arrow::field("dn_dt_agg", arrow::float64()),
        arrow::field("dn_dt_turb", arrow::float64()),
        arrow::field("heat_rate", arrow::float64()),
        arrow::field("d_heat_rate", arrow::float64()),
        arrow::field("rf_sw", arrow::float64()),
        arrow::field("rf_lw", arrow::float64()),
        arrow::field("rf_net", arrow::float64()),
        arrow::field("cumul_heat", arrow::float64()),
        arrow::field("cumul_differential_heat", arrow::float64())
    });

    return arrow::Table::Make(schema, {
        ID, parentID, birthTime, centrelon, centrelat, centrealt, heading, length, width, depth,
        area_eff, sigma_yz, n_ice_per_m, n_ice_per_vol, iwc, plume_mass_per_m, r_ice_vol,
        tau_contrail, terminal_fall_speed, diffuse_h, diffuse_v, dn_dt_agg, dn_dt_turb, heat_rate,
        d_heat_rate, rf_sw, rf_lw, rf_net, cumul_heat, cumul_differential_heat
    });
}
#endif

arrow::Result<std::shared_ptr<arrow::Table>> bin_to_table(const std::string& filename) {
    arrow::Result<std::shared_ptr<arrow::Table>> result;

    int loadedPlumeModelID;
    {
        std::ifstream is(filename, std::ios::binary);
        if (!is) {
            std::cerr << "Cannot open file for reading: " + filename << std::endl;
            exit(EXIT_FAILURE);
        }
        cereal::BinaryInputArchive archive(is);
        
        // Load plume model ID
        archive(loadedPlumeModelID);

        // Load vector according to plume model
        switch (loadedPlumeModelID) {
            case PlumeModels::CACE.ID: {
                // Do nothing, CaCE should not be able to output
                break;
            }
            case PlumeModels::COCIP.ID: {
#ifdef WITH_COCIP
                std::cout << "Archived segment type: " << PlumeModels::COCIP.name << std::endl;
                std::vector<SegmentCoCiP> vec;
                // Load vector
                archive(vec);
                // Create table
                result = CoCiP_to_table(vec);
#else
            
                std::cerr << "Error: Contrail Manager has not been built with "
                          << PlumeModels::COCIP.name << std::endl;
                exit(EXIT_FAILURE);
#endif
                break;
            }
            default: {
                std::cerr << "Plume model ID " << loadedPlumeModelID << " not recognised."
                    << std::endl;
                exit(EXIT_FAILURE);
                break;
            }
        }
    }

    return result;
}