set(EXECUTABLE solver1927)

# Solver1927 source files
file(GLOB SRC_LIST
    solver1927.cpp )
file(GLOB HEADERS
    solver1927.hpp
    memory_pool.hpp
    )

# Include directories
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(..)

# Create static library
ADD_LIBRARY(${EXECUTABLE} STATIC ${SRC_LIST} ${HEADERS})
TARGET_LINK_LIBRARIES(${EXECUTABLE})

# Installation
install( TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )
install( FILES ${HEADERS} DESTINATION include/${EXECUTABLE} )