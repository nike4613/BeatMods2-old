if (WIN32 AND NOT ${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC) # make restbed not do some dumb shit
	set(OLD_CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_WIN32_WINNT=0x0601 /W4 /wd4068 /wd4702")
endif()

set(DEPS_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/restbed/source")
include_directories(SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/restbed/source")
add_subdirectory ("restbed")

if (OLD_CMAKE_CXX_FLAGS)
	set(CMAKE_CXX_FLAGS ${OLD_CMAKE_CXX_FLAGS})
endif()

set(SKIP_PQXX_SHARED ON)
#set(SKIP_BUILD_TEST ON)
set(DEPS_INCLUDE_DIRS "${DEPS_INCLUDE_DIRS};${CMAKE_CURRENT_SOURCE_DIR}/libpqxx/include;${CMAKE_CURRENT_BINARY_DIR}/libpqxx/include")
include_directories(SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/libpqxx/include" "${CMAKE_CURRENT_BINARY_DIR}/libpqxx/include")
add_subdirectory("libpqxx")

set(DEPS_INCLUDE_DIRS "${DEPS_INCLUDE_DIRS}" PARENT_SCOPE)