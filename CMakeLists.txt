cmake_minimum_required(VERSION 3.10)
project(zmq_interface)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Python COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 CONFIG REQUIRED)
find_package(ZeroMQ REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(cppzmq REQUIRED)

find_path(CPPZMQ_INCLUDE_DIR zmq.hpp)
# Set output directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# Add source files (adjust the path according to your source structure)
set(SOURCES
    src/zmq_client.cpp
    src/zmq_message.cpp
    src/zmq_server.cpp
    src/data_topic.cpp
    src/pybind.cpp
)

# Create the pybind11 module
pybind11_add_module(zmq_interface ${SOURCES})

add_compile_options(-pthread)

target_link_libraries(zmq_interface PRIVATE
    libzmq
    spdlog::spdlog
    Threads::Threads
    pthread
)

# Include directories
target_include_directories(zmq_interface PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    $ENV{CONDA_PREFIX}/include
)

# Set properties for the target
set_target_properties(zmq_interface PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_SYSTEM_PROCESSOR}"
    POSITION_INDEPENDENT_CODE ON
)

# Install rules

# if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm.*|arm|aarch64")
#     set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../lib/aarch64)

# elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64)|(AMD64)|(^i686)")
#     set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../lib/x86_64)
# endif()

set_target_properties(zmq_interface PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(zmq_interface PROPERTIES PREFIX "")
set_target_properties(zmq_interface PROPERTIES OUTPUT_NAME "zmq_interface")
