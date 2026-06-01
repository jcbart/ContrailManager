#include <iostream>
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <arrow/csv/writer.h>
#include "readbin.h"

// Write Arrow table to CSV file; replaces the file extension on filename with ".csv"
arrow::Status table_to_csv(const std::shared_ptr<arrow::Table>& table,
    const std::filesystem::path& filepath) {

    std::filesystem::path outfilepath = std::filesystem::path(filepath);
    outfilepath.replace_extension(".csv");

    std::cout << "Saving to " << outfilepath << std::endl;
    
    ARROW_ASSIGN_OR_RAISE(auto outstream, arrow::io::FileOutputStream::Open(outfilepath.string()));

    ARROW_RETURN_NOT_OK(arrow::csv::WriteCSV(
        *table,
        arrow::csv::WriteOptions::Defaults(),
        outstream.get()
    ));

    return outstream->Close();
}

int main(int argc, char* argv[]) {
    read_and_emit_bin(argc, argv, table_to_csv);
    return 0;
}