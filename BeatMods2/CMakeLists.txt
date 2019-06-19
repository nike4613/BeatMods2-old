# CMakeList.txt : CMake project for BeatMods2, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (BeatMods2 
	"include/BeatMods2.h"		"src/BeatMods2.cpp"
	"include/db/engine.h"		"src/db/engine.cpp"
	"include/logging/Logging.h" "src/logging/Logging.cpp"
	"include/util/json.h"
	"include/util/config.h"		"src/util/config.cpp")
target_include_directories(BeatMods2 PRIVATE "include")
target_link_libraries (BeatMods2 restbed-static pqxx_static semver)
set_property(TARGET BeatMods2 PROPERTY CXX_STANDARD 17)
set_property(TARGET BeatMods2 PROPERTY CXX_STANDARD_REQUIRED ON)

# TODO: Add tests and install targets if needed.
