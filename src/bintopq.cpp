#include <iostream>
#include <filesystem>
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/writer.h>
#include "readbin.h"

// Write Arrow table to Parquet file; replaces the file extension on filename with ".pq"
arrow::Status table_to_pq(const std::shared_ptr<arrow::Table>& table,
    const std::string& filename) {

    std::string outfilename = std::filesystem::path(filename).replace_extension(".pq").string();

    std::cout << "Saving to " << outfilename << std::endl;

    std::shared_ptr<parquet::WriterProperties> props = 
        parquet::WriterProperties::Builder().compression(arrow::Compression::SNAPPY)->build();

    std::shared_ptr<parquet::ArrowWriterProperties> arrow_props = 
        parquet::ArrowWriterProperties::Builder().store_schema()->build();
    
    ARROW_ASSIGN_OR_RAISE(auto outstream, arrow::io::FileOutputStream::Open(outfilename));

    constexpr int64_t chunk_size = 64 * 1024;

    ARROW_RETURN_NOT_OK(parquet::arrow::WriteTable(
        *table.get(),
        arrow::default_memory_pool(),
        outstream,
        chunk_size,
        props,
        arrow_props
    ));

    return outstream->Close();
}

int main(int argc, char* argv[]) {
    read_and_emit_bin(argc, argv, table_to_pq);
    return 0;
}