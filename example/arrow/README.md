# Apache Arrow HFC Example

Demonstrates using [Apache Arrow](https://github.com/apache/arrow) v17.0.0 with HermeticFetchContent.

## Build
cmake-re build:
```bash
cmake-re -GNinja -S . -B build_re -DCMAKE_TOOLCHAIN_FILE=environments/linux-tipi-ubuntu-cxx17.cmake -DCMAKE_BUILD_TYPE=Release
cmake-re --build build_re
```

cmake build : 
```bash
cmake -GNinja -S . -B build_cmake -DCMAKE_TOOLCHAIN_FILE=environments/linux-tipi-ubuntu-cxx17.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build ./build_cmake
```

## What it does

The example creates Arrow arrays and a record batch, then uses Arrow Compute to calculate the sum of an integer column.