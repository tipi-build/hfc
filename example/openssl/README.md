# Adding OpenSSL to your project using HFC

This example showcases how to add [OpenSSL](https://github.com/openssl/openssl) to your HFC project.

```
cmake -GNinja -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/main
```

To run the same build with `cmake-re` instead (as a local containerized build in this case):

```
cmake-re -GNinja -S . -B build_re -DCMAKE_TOOLCHAIN_FILE=environments/linux-tipi-ubuntu-cxx17.cmake -DCMAKE_BUILD_TYPE=Release
```

The example application uses OpenSSL to compute the SHA-256 hash of a string. This examples shows how to create additional CMake targets using `HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION`. Here we have added the following targets:

- OpenSSL::Crypto
- OpenSSL::SSL