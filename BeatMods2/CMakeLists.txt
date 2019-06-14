# CMakeList.txt : CMake project for BeatMods2, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (BeatMods2 "src/BeatMods2.cpp" "include/BeatMods2.h")
target_include_directories(BeatMods2 PRIVATE "include")

# TODO: Add tests and install targets if needed.
