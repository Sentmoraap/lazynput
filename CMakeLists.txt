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

SET_PROPERTY(TARGET ${CURRENT_TARGET} PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/include/
)
