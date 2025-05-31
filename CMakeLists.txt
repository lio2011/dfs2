cmake_minimum_required(VERSION 3.10)
project(dfs2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenSSL (if you use it in your project)
find_package(OpenSSL REQUIRED)

# Find Protobuf and gRPC
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${gRPC_INCLUDE_DIRS})

# Proto files
file(GLOB PROTO_FILES "${CMAKE_SOURCE_DIR}/include/*.proto")

# Generate gRPC and protobuf sources
foreach(proto_file ${PROTO_FILES})
    get_filename_component(proto_name ${proto_file} NAME_WE)
    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${proto_file})
    find_package(gRPC CONFIG REQUIRED)
    # include(GRPCGenerate)
    # grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${proto_file})
    list(APPEND ALL_PROTO_SRCS ${PROTO_SRCS} ${GRPC_SRCS})
    list(APPEND ALL_PROTO_HDRS ${PROTO_HDRS} ${GRPC_HDRS})
endforeach()

# Collect all cpp files in src/
file(GLOB SRC_FILES src/*.cpp include/*.pb.cc include/*.grpc.pb.cc)

# Create the executable from all source files
add_executable(main_exec ${SRC_FILES})

# Link OpenSSL if needed
target_link_libraries(main_exec
    OpenSSL::Crypto
    gRPC::grpc++
    ${Protobuf_LIBRARIES}
)