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

# Add source files with new directory structure
set(SOURCES
    zmq_interface/core/src/zmq_client.cpp
    zmq_interface/core/src/zmq_message.cpp
    zmq_interface/core/src/zmq_server.cpp
    zmq_interface/core/src/data_topic.cpp
    zmq_interface/core/src/common.cpp
    zmq_interface/core/src/pybind.cpp
)

# Create the pybind11 module with the new target name
pybind11_add_module(zmq_interface_core ${SOURCES})

add_compile_options(-pthread)

target_link_libraries(zmq_interface_core PRIVATE
    libzmq
    spdlog::spdlog
    Threads::Threads
    pthread
)

# Update include directories for new structure
target_include_directories(zmq_interface_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/zmq_interface/core/include
)


# Update the output location and name
set_target_properties(zmq_interface_core PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/zmq_interface/core"
    PREFIX ""
    OUTPUT_NAME "zmq_interface"
)