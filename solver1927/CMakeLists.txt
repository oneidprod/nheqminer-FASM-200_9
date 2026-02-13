cmake_minimum_required(VERSION 3.8)
project(Solver1927 CXX)

# Enable AVX2 for SIMD operations
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2 -std=c++17 -O2")

set(EXECUTABLE solver1927)

# Solver1927 source files
file(GLOB SRC_LIST
    solver1927.cpp 
    simd_detector.cpp 
    blake2b_hasher.cpp 
    collision_detector.cpp
    ../blake2/blake2bx.cpp)
file(GLOB HEADERS
    solver1927.hpp
    memory_pool.hpp
    simd_detector.hpp
    blake2b_hasher.hpp
    collision_detector.hpp
    )

# Include directories
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(..)
include_directories(../blake2)

# Create static library
ADD_LIBRARY(${EXECUTABLE} STATIC ${SRC_LIST} ${HEADERS})
TARGET_LINK_LIBRARIES(${EXECUTABLE})

# Test executable target
ADD_EXECUTABLE(test main.cpp simd_detector.cpp blake2b_hasher.cpp collision_detector.cpp solver1927.cpp ../blake2/blake2bx.cpp)
TARGET_LINK_LIBRARIES(test)

# Installation
install( TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )
install( FILES ${HEADERS} DESTINATION include/${EXECUTABLE} )