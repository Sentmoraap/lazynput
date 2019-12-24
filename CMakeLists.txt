## CMakeLists.txt
##
##

# Define the following vars in your CMakeCache.txt file(s)

cmake_minimum_required(VERSION 2.6)

PROJECT(Lazynput)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -pedantic")

FILE(
    GLOB_RECURSE
    source_files
    src/*
)

#ADD_LIBRARY(Lazynput SHARED ${source_files})
ADD_EXECUTABLE(Lazynput ${source_files})

SET(CURRENT_TARGET Lazynput)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

FIND_PACKAGE(SFML 2.5.0 COMPONENTS graphics window system REQUIRED)

IF(SFML_FOUND)
  TARGET_LINK_LIBRARIES(${CURRENT_TARGET} ${SFML_LIBRARIES})
ENDIF(SFML_FOUND)

SET_PROPERTY(TARGET ${CURRENT_TARGET} PROPERTY INCLUDE_DIRECTORIES
  ${SFML_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}/include/
)
