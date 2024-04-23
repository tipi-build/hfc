# Build

```
cmake -GNinja -S . -B build -DCMAKE_TOOLCHAIN_FILE=../../toolchain/toolchain.cmake
cmake --build build
```