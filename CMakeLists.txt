cmake_minimum_required(VERSION 3.10)
project(dfs2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenSSL (if you use it in your project)
find_package(OpenSSL REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Collect all cpp files in src/
file(GLOB SRC_FILES src/*.cpp)

# Create the executable from all source files
add_executable(main_exec ${SRC_FILES})

# Link OpenSSL if needed
target_link_libraries(main_exec OpenSSL::Crypto)