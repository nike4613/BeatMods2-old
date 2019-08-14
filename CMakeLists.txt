# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("BeatMods2" VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include sub-projects.
add_subdirectory ("extern" EXCLUDE_FROM_ALL)
include_directories(SYSTEM ${DEPS_INCLUDE_DIRS})
add_subdirectory ("BeatMods2")
