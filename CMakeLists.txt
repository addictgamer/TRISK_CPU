cmake_minimum_required(VERSION 2.6)
project(TRISK_F2016)
#Tak's Risc Processor

#tem -- toyprocessor emulator
#tas -- toyprocessor assembler

if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -g -std=c++17")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++17")

find_package(CXX11 REQUIRED)
set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX11_FLAGS}")
#set ( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CXX11_FLAGS}")
#set ( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${CXX11_FLAGS}")
#set ( CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${CXX11_FLAGS}")

# Add include directories

# Add the source directory
file(GLOB_RECURSE EMULATOR_FILES src/emulator/*.cpp src/emulator/*.hpp)
file(GLOB_RECURSE ASSEMBLER_FILES src/assembler/*.cpp src/assembler/*.hpp)

add_executable(tem ${EMULATOR_FILES})
add_executable(tas ${ASSEMBLER_FILES})
