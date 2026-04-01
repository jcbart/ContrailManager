#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <arrow/csv/writer.h>
#include "bintotable.h"

// Write Arrow table to CSV file; replaces the file extension on filename with ".csv"
arrow::Status table_to_csv(const std::shared_ptr<arrow::Table>& table,
    const std::string& filename) {

    auto outfilename = std::filesystem::path(filename).replace_extension(".csv").string();
    
    ARROW_ASSIGN_OR_RAISE(auto outstream, arrow::io::FileOutputStream::Open(outfilename));

    ARROW_RETURN_NOT_OK(arrow::csv::WriteCSV(
        *table,
        arrow::csv::WriteOptions::Defaults(),
        outstream.get()
    ));

    return outstream->Close();
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        std::cerr << "Error: specify file path in command line." << std::endl;
        exit(EXIT_FAILURE);
    }
    if (argc > 2) {
        std::cerr << "Error: specify only one file path in command line." << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string filename = argv[1];

    // Read file and convert archived segments to Arrow table
    arrow::Result<std::shared_ptr<arrow::Table>> result = bin_to_table(filename);

    if (!result.ok()) {
        std::cerr << "Arrow error: " << result.status().message() << std::endl;
        exit(EXIT_FAILURE);
    }

    // Get table from result
    std::shared_ptr<arrow::Table> table = result.ValueOrDie();

    // Save to CSV
    arrow::Status status = table_to_csv(table, filename);

    if (!status.ok()) {
        std::cerr << "Arrow error: " << status.message() << std::endl;
        exit(EXIT_FAILURE);
    }

    return 0;
}