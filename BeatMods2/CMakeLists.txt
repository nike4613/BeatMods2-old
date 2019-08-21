# CMakeList.txt : CMake project for BeatMods2, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

include(gql-api.cmake)

# Add source to this project's executable.
add_executable (BeatMods2 # for consistency, can we please keep the inlcude files on the left?
	"include/BeatMods2.h"       "src/BeatMods2.cpp"
	"include/db/engine.h"       "src/db/engine.cpp"
	"include/logging/Logging.h" "src/logging/Logging.cpp"
	"include/util/json.h"
	"include/util/algorithm.h"
	"include/util/config.h"     "src/util/config.cpp"
	"include/util/semver.h"     "src/util/semver.cpp"
	"include/auth/github.h"     "src/auth/github.cpp"
	
	"${CMAKE_CURRENT_BINARY_DIR}/generated/Schema.h"
	"${CMAKE_CURRENT_BINARY_DIR}/generated/Schema.cpp"
	)
target_include_directories(BeatMods2 PRIVATE "include")
target_link_libraries (BeatMods2 restbed-static pqxx_static tz date_interface libgraphqlservice)
add_dependencies(BeatMods2 RapidJSON)
set_property(TARGET BeatMods2 PROPERTY CXX_STANDARD 17)
set_property(TARGET BeatMods2 PROPERTY CXX_STANDARD_REQUIRED ON)

# TODO: Add tests and install targets if needed.
