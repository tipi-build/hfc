# Fetching GRPC with HFC and generating protobuf stubs

## Goal

This example builds protobuf and gRPC from scratch. It's based on [gRPC's Hello World example](https://github.com/grpc/grpc/tree/master/examples/cpp/helloworld) but instead of using a version of protobuf in the system it builds protobuf and grpc from source.

This shows how to setup a complex dependency tree as well as make 3rd party packages fetched via HermeticFetchContent work well with find_package, both when used by other dependencies or the main project's build itself.

## How to run

You will not need any additional dependencies aside from cmake itself as protoc and all the required libraries are added during the build. To build the project you can run:

```sh
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## How it works

### Setting up find_package for a dependency

Sometimes you need to use macros and functions from a cmake module in a dependency. This is the case for `protobuf_generate` which is a module within the protobuf dependency. This will not work out of the box though with HermeticFetchContent, instead we'll need to configure some variables to find where the project is located:

```cmake
include(hfc_provide_dependency_FINDPACKAGE)
hfc_provide_dependency_FINDPACKAGE("FIND_PACKAGE" Protobuf)
```

Invoking this function sets up `PROTOBUF_ROOT_DIR`, which gives us access to the path of the install tree of protobuf. This way we can now register the module:

```cmake
set(Protobuf_CMAKE_MODULES_PATH "${PROTOBUF_ROOT_DIR}/lib/cmake/protobuf")
list(APPEND CMAKE_MODULE_PATH "${Protobuf_CMAKE_MODULES_PATH}")
```

This makes it possible to run `include(protobuf-generate)` which will find `protobuf_generate`.

### Using an executable from a dependency

Similarly to what we have just done, we can use this technique to get the path to an executable. In this case we can take the path to `protoc`, ensuring that calls to `protobuf_generate` use the `protoc` binary we have just built:

```cmake
set(Protobuf_PROTOC_EXECUTABLE "$<TARGET_FILE:protobuf::protoc>")
protobuf_generate(
    TARGET grpc-stubs
    IMPORT_DIRS ${PROTO_IMPORT_DIRS}
    PROTOC_EXE "${Protobuf_PROTOC_EXECUTABLE}"
    PROTOC_OUT_DIR "${PROTO_BINARY_DIR}")
```

## Issues

- gRPC does not build when `CMAKE_BUILD_TYPE=Debug` so we force the option in its `HERMETIC_TOOLCHAIN_EXTENSION`