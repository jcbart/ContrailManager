#include <memory>
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <arrow/compute/api.h>
#include <parquet/arrow/reader.h>
#include "FlightContainer.h"
#include "timekeeping.h"
#include "CMLog.h"

// Struct to hold parquet column and details
struct ParquetColumn {
    // Column index of column in table
    int colIndex;
    // Alias matched with column name
    std::string matchedName;

    ParquetColumn(const int colIndex, const std::string& matchedName)
        : colIndex(colIndex), matchedName(matchedName) {}
};

// Given an arrow::Table plus aliases and valid types for a field, returns a ParquetColumn holding
// the column's details
ParquetColumn find_parquet_field(std::shared_ptr<arrow::Table>& table,
    const std::vector<std::string>& aliases,
    const std::vector<std::shared_ptr<arrow::DataType>>& valid_types) {
    
    // Loop through aliases
    for (const std::string& alias : aliases) {
        int col_idx = table->schema()->GetFieldIndex(alias);
        if (col_idx != -1) {
            // If column found, loop through valid types
            for (const std::shared_ptr<arrow::DataType>& type : valid_types) {
                if (table->column(col_idx)->type()->Equals(type)) {
                    return ParquetColumn(col_idx, alias);
                }
            }
            std::string err;
            err = "While reading parquet file, found column " + alias + " with type "
                + table->column(col_idx)->type()->ToString()
                + " which is not a member of valid types: [ ";
            for (size_t i = 0; i < valid_types.size(); i++) {
                err += valid_types[i]->ToString() + " ";
            }
            err += "].";
            CM_RaiseError(err, __FILE__, __LINE__);
        }
    }
    std::string err = "No column found which matches aliases: [ ";
    for (size_t i = 0; i < aliases.size(); i++) {
        err += aliases[i] + " ";
    }
    err += "].";
    CM_RaiseError(err, __FILE__, __LINE__);
    return ParquetColumn(-1, "");
}

// Converts an Arrow timestamp value given its unit to CMTime
inline CMTime arrow_timestamp_to_CMTime(
    int64_t timestamp_value, arrow::TimeUnit::type unit
) {
    using namespace std::chrono;
    switch (unit) {
        case arrow::TimeUnit::SECOND:
            return CMTime(sys_time<milliseconds>{
                duration_cast<milliseconds>(seconds{timestamp_value})
            });
        case arrow::TimeUnit::MILLI:
            return CMTime(sys_time<milliseconds>{
                duration_cast<milliseconds>(milliseconds{timestamp_value})
            });
        case arrow::TimeUnit::MICRO:
            return CMTime(sys_time<milliseconds>{
                duration_cast<milliseconds>(microseconds{timestamp_value})
            });
        case arrow::TimeUnit::NANO:
            return CMTime(sys_time<milliseconds>{
                duration_cast<milliseconds>(nanoseconds{timestamp_value})
            });
        default:
            // For safety, though Arrow defines no other type
            return CMTime(sys_time<milliseconds>{milliseconds{0}});
    }
}

// Opens a parquet file as an arrow::Table; returns status
arrow::Status parquet_to_table(const std::string& filepath, std::shared_ptr<arrow::Table>& table) {
    arrow::MemoryPool* pool = arrow::default_memory_pool();
    std::shared_ptr<arrow::io::RandomAccessFile> input;
    ARROW_ASSIGN_OR_RAISE(input, arrow::io::ReadableFile::Open(filepath));

    // Open Parquet file reader
    std::unique_ptr<parquet::arrow::FileReader> arrow_reader;
    ARROW_ASSIGN_OR_RAISE(arrow_reader, parquet::arrow::OpenFile(input, pool));

    // Read entire file as a single Arrow table
    ARROW_RETURN_NOT_OK(arrow_reader->ReadTable(&table));

    return arrow::Status::OK();
}

// Cast an array to type arrow::float64 (double) if its type is compatible
inline std::shared_ptr<arrow::Array> toDoubleArray(const std::shared_ptr<arrow::Array>& array) {
    if (array->type()->Equals(arrow::float64())) {
        return array;
    }
    arrow::Result<arrow::Datum> result = arrow::compute::Cast(array, arrow::float64());
    if (!result.ok()) {
        CM_RaiseError("Arrow error: " + result.status().ToString(), __FILE__, __LINE__);
    }
    return result->make_array();
}

// Cast an array to type arrow::utf8 (string) if its type is compatible
inline std::shared_ptr<arrow::Array> toStringArray(const std::shared_ptr<arrow::Array>& array) {
    if (array->type()->Equals(arrow::utf8())) {
        return array;
    }
    arrow::Result<arrow::Datum> result = arrow::compute::Cast(array, arrow::utf8());
    if (!result.ok()) {
        CM_RaiseError("Arrow error: " + result.status().ToString(), __FILE__, __LINE__);
    }
    return result->make_array();
}

void FlightContainer::readParquet(const std::string& filepath) {
    // Arrow table of parquet file
    std::shared_ptr<arrow::Table> table;
    arrow::Status status = parquet_to_table(filepath, table);
    if (!status.ok()) {
        CM_RaiseError("Arrow error: " + status.message(), __FILE__, __LINE__);
    }

    // Reader to read table in batches
    arrow::TableBatchReader reader(*table);
    //reader.set_chunksize(1024); // If needed

    int64_t nrows = table->num_rows();
    int64_t ncols = table->num_columns();

    if (nrows == 0 || ncols == 0) {
        CM_RaiseError("Flight dataset is empty!", __FILE__, __LINE__);
    }

    // Get each parquet column

    ParquetColumn flight_id = find_parquet_field(
        table,
        {
            "id",
            "flight_id",
            "flightid"
        },
        {
            arrow::utf8(),
            arrow::large_utf8()
        }
    );
    ParquetColumn timestamp = find_parquet_field(
        table,
        {
            "time",
            "time_utc",
            "timestamp"
        },
        {
            arrow::timestamp(arrow::TimeUnit::NANO),
            arrow::timestamp(arrow::TimeUnit::MICRO),
            arrow::timestamp(arrow::TimeUnit::MILLI),
            arrow::timestamp(arrow::TimeUnit::SECOND)
        }
    );
    ParquetColumn lon = find_parquet_field(
        table,
        {
            "lon",
            "long",
            "longitude",
            "lon_deg",
            "long_deg",
            "longitude_deg"
        },
        {
            arrow::float64(),
            arrow::float32()
        }
    );
    ParquetColumn lat = find_parquet_field(
        table,
        {
            "lat",
            "latitude",
            "lat_deg",
            "latitude_deg"
        },
        {
            arrow::float64(),
            arrow::float32()
        }
    );
    ParquetColumn alt = find_parquet_field(
        table,
        {
            "alt",
            "alt_m",
            "altitude",
            "altitude_m"
        },
        {
            arrow::float64(),
            arrow::float32()
        }
    );
    ParquetColumn aircraft_mass = find_parquet_field(
        table,
        {
            "aircraft_mass",
        },
        {
            arrow::float64(),
            arrow::float32()
        }
    );
    ParquetColumn fuel_flow = find_parquet_field(
        table,
        {
            "fuel_flow",
        },
        {
            arrow::float64(),
            arrow::float32()
        }
    );
    ParquetColumn engine_efficiency = find_parquet_field(
        table,
        {
            "engine_efficiency",
        },
        {
            arrow::float64(),
            arrow::float32()
        }
    );
    ParquetColumn nvpm_ei_n = find_parquet_field(
        table,
        {
            "nvpm_ei_n",
        },
        {
            arrow::float64(),
            arrow::float32()
        }
    );

    // Arrow DataType of timestamp column
    std::shared_ptr<arrow::TimestampType> timestamp_type
        = std::static_pointer_cast<arrow::TimestampType>(table->column(timestamp.colIndex)->type());

    // Unit of timestamp column
    arrow::TimeUnit::type time_unit = timestamp_type->unit();

    std::shared_ptr<arrow::RecordBatch> batch;
    // Iterate through batches
    while (reader.ReadNext(&batch).ok() && batch != nullptr) {
        // Get pointers to arrays from each batch

        const std::shared_ptr<arrow::StringArray> flight_id_array
            = std::static_pointer_cast<arrow::StringArray>(toStringArray(batch->column(flight_id.colIndex)));

        const std::shared_ptr<arrow::TimestampArray> timestamp_array
            = std::static_pointer_cast<arrow::TimestampArray>(batch->column(timestamp.colIndex));

        const std::shared_ptr<arrow::DoubleArray> lon_array
            = std::static_pointer_cast<arrow::DoubleArray>(toDoubleArray(batch->column(lon.colIndex)));

        const std::shared_ptr<arrow::DoubleArray> lat_array
            = std::static_pointer_cast<arrow::DoubleArray>(toDoubleArray(batch->column(lat.colIndex)));

        const std::shared_ptr<arrow::DoubleArray> alt_array
            = std::static_pointer_cast<arrow::DoubleArray>(toDoubleArray(batch->column(alt.colIndex)));

        const std::shared_ptr<arrow::DoubleArray> aircraft_mass_array
            = std::static_pointer_cast<arrow::DoubleArray>(toDoubleArray(batch->column(aircraft_mass.colIndex)));

        const std::shared_ptr<arrow::DoubleArray> fuel_flow_array
            = std::static_pointer_cast<arrow::DoubleArray>(toDoubleArray(batch->column(fuel_flow.colIndex)));

        const std::shared_ptr<arrow::DoubleArray> engine_efficiency_array
            = std::static_pointer_cast<arrow::DoubleArray>(toDoubleArray(batch->column(engine_efficiency.colIndex)));

        const std::shared_ptr<arrow::DoubleArray> nvpm_ei_n_array
            = std::static_pointer_cast<arrow::DoubleArray>(toDoubleArray(batch->column(nvpm_ei_n.colIndex)));

        // Process each row in batch
        for (int64_t row_idx = 0; row_idx < batch->num_rows(); row_idx++) {
            if (flight_id_array->IsNull(row_idx)) {
                continue; // Ignore null
            }

            // Flight ID
            std::string id = flight_id_array->GetString(row_idx);

            // Check if id is in map
            auto it = flightIDIndexMap.find(id);
            // Index of flight in FlightContainer::loaded
            size_t loaded_idx;

            if (it == flightIDIndexMap.end()) {
                // New flight - add to vector
                loaded.emplace_back(id);
                // Get index
                loaded_idx = loaded.size() - 1;
                // Add to index map
                flightIDIndexMap[id] = loaded_idx;
            }
            else {
                // Existing flight - get its index
                loaded_idx = it->second;
            }

            // Add row data to flight at loaded_idx
            Flight& flight = loaded[loaded_idx];

            if (timestamp_array->IsValid(row_idx) &&
                lon_array->IsValid(row_idx) &&
                lat_array->IsValid(row_idx) &&
                alt_array->IsValid(row_idx) &&
                aircraft_mass_array->IsValid(row_idx) &&
                fuel_flow_array->IsValid(row_idx) &&
                engine_efficiency_array->IsValid(row_idx) &&
                nvpm_ei_n_array->IsValid(row_idx)) {
                
                // Convert timestamp and location to CMTime and Geo3D and add to flight
                flight.waypoints.emplace_back(
                    arrow_timestamp_to_CMTime(
                        timestamp_array->Value(row_idx),
                        time_unit
                    ),
                    Geo3D(
                        lon_array->Value(row_idx),
                        lat_array->Value(row_idx),
                        alt_array->Value(row_idx)
                    ),
                    aircraft_mass_array->Value(row_idx),
                    fuel_flow_array->Value(row_idx),
                    engine_efficiency_array->Value(row_idx),
                    nvpm_ei_n_array->Value(row_idx)
                );
            }
        }
    }
}