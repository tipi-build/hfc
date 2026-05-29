#include <arrow/api.h>
#include <arrow/io/file.h>
#include <parquet/arrow/reader.h>
#include <parquet/arrow/writer.h>

#include <iostream>

arrow::Status run() {
  // Build a simple Int64 array
  arrow::Int64Builder int_builder;
  ARROW_RETURN_NOT_OK(int_builder.AppendValues({1, 2, 3, 4, 5}));
  std::shared_ptr<arrow::Array> int_array;
  ARROW_RETURN_NOT_OK(int_builder.Finish(&int_array));

  // Build a String array
  arrow::StringBuilder str_builder;
  ARROW_RETURN_NOT_OK(
      str_builder.AppendValues({"Alice", "Bob", "Charlie", "Diana", "Eve"}));
  std::shared_ptr<arrow::Array> str_array;
  ARROW_RETURN_NOT_OK(str_builder.Finish(&str_array));

  // Create a schema and a table
  auto schema = arrow::schema(
      {arrow::field("name", arrow::utf8()), arrow::field("score", arrow::int64())});

  auto table = arrow::Table::Make(schema, {str_array, int_array});

  std::cout << "=== Arrow Table ===" << std::endl;
  std::cout << "Schema: " << table->schema()->ToString() << std::endl;
  std::cout << "Rows: " << table->num_rows() << std::endl;
  std::cout << table->ToString() << std::endl;

  auto typed_array = std::static_pointer_cast<arrow::Int64Array>(int_array);
  int64_t sum = 0;
  for (int64_t i = 0; i < typed_array->length(); ++i) {
    sum += typed_array->Value(i);
  }
  std::cout << "Sum of scores: " << sum << std::endl;

  // Write the table to a Parquet file
  const std::string parquet_path = "example.parquet";
  ARROW_ASSIGN_OR_RAISE(auto outfile,
                        arrow::io::FileOutputStream::Open(parquet_path));
  ARROW_RETURN_NOT_OK(
      parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), outfile, 1024));
  std::cout << "\n=== Written to " << parquet_path << " ===" << std::endl;

  // Read the Parquet file back
  ARROW_ASSIGN_OR_RAISE(auto infile, arrow::io::ReadableFile::Open(parquet_path));
  std::unique_ptr<parquet::arrow::FileReader> reader;
  ARROW_RETURN_NOT_OK(
      parquet::arrow::OpenFile(infile, arrow::default_memory_pool(), &reader));

  std::shared_ptr<arrow::Table> read_table;
  ARROW_RETURN_NOT_OK(reader->ReadTable(&read_table));

  std::cout << "\n=== Read back from Parquet ===" << std::endl;
  std::cout << "Schema: " << read_table->schema()->ToString() << std::endl;
  std::cout << "Rows: " << read_table->num_rows() << std::endl;
  std::cout << read_table->ToString() << std::endl;

  return arrow::Status::OK();
}

int main() {
  auto status = run();
  if (!status.ok()) {
    std::cerr << "Error: " << status.ToString() << std::endl;
    return 1;
  }
  return 0;
}
