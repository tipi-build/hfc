cmake_minimum_required(VERSION 3.27.6)
project(grpc_use_3rd_party_cmake_modules)

include(thirdparty/build_thirdparty.cmake)

set(GRPC_STUB_LIBRARIES "protobuf::libprotobuf;gRPC::grpc++;gRPC::grpc++_reflection;gRPC::grpc_unsecure")

add_library(grpc-stubs OBJECT "proto/greeter.proto")
target_link_libraries(grpc-stubs PUBLIC ${GRPC_STUB_LIBRARIES})

set(PROTO_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_include_directories(grpc-stubs PUBLIC "$<BUILD_INTERFACE:${PROTO_BINARY_DIR}>")

set(PROTO_IMPORT_DIRS "${CMAKE_CURRENT_LIST_DIR}/proto")

protobuf_generate(
    TARGET grpc-stubs
    IMPORT_DIRS ${PROTO_IMPORT_DIRS}
    PROTOC_EXE "${Protobuf_PROTOC_EXECUTABLE}"
    PROTOC_OUT_DIR "${PROTO_BINARY_DIR}")

protobuf_generate(
    TARGET grpc-stubs
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=\$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
    IMPORT_DIRS ${PROTO_IMPORT_DIRS}
    PROTOC_EXE "${Protobuf_PROTOC_EXECUTABLE}"
    PROTOC_OUT_DIR "${PROTO_BINARY_DIR}")

add_executable(server server.cpp)
target_link_libraries(server PUBLIC grpc-stubs ${GRPC_STUB_LIBRARIES})
target_include_directories(server PUBLIC "$<BUILD_INTERFACE:${PROTO_BINARY_DIR}>")

add_executable(client client.cpp)
target_link_libraries(client PUBLIC grpc-stubs ${GRPC_STUB_LIBRARIES})
target_include_directories(client PUBLIC "$<BUILD_INTERFACE:${PROTO_BINARY_DIR}>")
