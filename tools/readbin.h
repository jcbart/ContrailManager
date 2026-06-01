#ifndef BINTOTABLE_H
#define BINTOTABLE_H

#include <memory>
#include <string>
#include <filesystem>
#include <arrow/api.h>

// Read file and convert archived segments to Arrow table
arrow::Result<std::shared_ptr<arrow::Table>> bin_to_table(const std::filesystem::path& filepath);

// Read bin file from command line, convert to table, and call an
// `emit` function (of the form `table_to_X`)
template <typename Emit>
inline void read_and_emit_bin(int argc, char* argv[], Emit&& emit) {
    if (argc < 2) {
        std::cerr << "Error: specify file path in command line." << std::endl;
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; i++) {
        const std::filesystem::path filepath(argv[i]);

        // Read file and convert archived segments to Arrow table
        arrow::Result<std::shared_ptr<arrow::Table>> result = bin_to_table(filepath);

        if (!result.ok()) {
            std::cerr << "Arrow error: " << result.status().message() << std::endl;
            exit(EXIT_FAILURE);
        }

        // Get table from result
        std::shared_ptr<arrow::Table> table = result.ValueOrDie();

        // Save to file through emit function
        arrow::Status status = emit(table, filepath);

        if (!status.ok()) {
            std::cerr << "Arrow error: " << status.message() << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

#endif