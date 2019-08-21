if (WIN32 AND NOT ${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC) # make restbed not do some dumb shit
	set(OLD_CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_WIN32_WINNT=0x0601 /W4 /wd4068 /wd4702")
endif()

if (APPLE)
	set(OPENSSL_ROOT_DIR /usr/local/opt/openssl/)
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

set(DEPS_INCLUDE_DIRS "${DEPS_INCLUDE_DIRS};${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include")

set(USE_SYSTEM_TZ_DB ON)
set(ENABLE_DATE_TESTING OFF)
add_subdirectory("date")

# curl stuff
#set(CURL_STATICLIB true)
#set(BUILD_CURL_EXE false)
#set(HTTP_ONLY true)
#set(LIBCURL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/curl)
#find_path(LIBCURL_INCLUDE_DIR curl/curl.h HINTS ${LIBCURL_DIR}/include)
#set(LIBS ${LIBS} libcurl)
#set(DEPS_INCLUDE_DIRS "${DEPS_INCLUDE_DIRS};${LIBCURL_INCLUDE_DIR}")
#add_definitions(-DCURL_STATICLIB)
#add_subdirectory("curl")

include(ExternalProject)

set(RapidJSON_INSTALL_PATH "${CMAKE_CURRENT_BINARY_DIR}/rapidjson/install")

ExternalProject_Add(RapidJSON
    PREFIX rapidjson
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson

    CMAKE_CACHE_ARGS "-DCMAKE_INSTALL_PREFIX:PATH=${RapidJSON_INSTALL_PATH}"

    # BUILD_BYPRODUCTS

    EXCLUDE_FROM_ALL true
)

set(RapidJSON_INCLUDE_DIR "${RapidJSON_INSTALL_PATH}/include")

set(GraphQLGen_INSTALL_PATH ${CMAKE_CURRENT_BINARY_DIR}/graphqlgen/install)
set(GraphQLGen_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/graphqlgen)

ExternalProject_Add(GraphQLGen
    PREFIX graphqlgen
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/graphqlgen

    CMAKE_ARGS "-DRAPIDJSON_DIR:PATH=${RapidJSON_INSTALL_PATH}"
    CMAKE_CACHE_ARGS "-DCMAKE_CXX_FLAGS:STRING=-isystem ${RapidJSON_INCLUDE_DIR} -static-libstdc++"

    CMAKE_CACHE_ARGS "-DGRAPHQL_UPDATE_SAMPLES:BOOL=OFF"
    CMAKE_CACHE_ARGS "-DGRAPHQL_BUILD_TESTS:BOOL=OFF"

    CMAKE_CACHE_ARGS "-DCMAKE_INSTALL_PREFIX:PATH=${GraphQLGen_INSTALL_PATH}"

    BUILD_BYPRODUCTS ${GraphQLGen_BUILD_DIR}/src/GraphQLGen-build/src/schemagen

    EXCLUDE_FROM_ALL true

    DEPENDS RapidJSON
)

add_library(libgraphqljson STATIC IMPORTED GLOBAL)
set_property(TARGET libgraphqljson PROPERTY IMPORTED_LOCATION "${GraphQLGen_INSTALL_PATH}/lib/libgraphqljson.a")
add_library(libgraphqlpeg STATIC IMPORTED GLOBAL)
set_property(TARGET libgraphqlpeg PROPERTY IMPORTED_LOCATION "${GraphQLGen_INSTALL_PATH}/lib/libgraphqlpeg.a")
add_library(libgraphqlservice STATIC IMPORTED GLOBAL)
set_property(TARGET libgraphqlservice PROPERTY IMPORTED_LOCATION "${GraphQLGen_INSTALL_PATH}/lib/libgraphqlservice.a")
add_dependencies(libgraphqlservice libgraphqljson libgraphqlpeg)

set(GraphQLGen_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/graphqlgen/include;${GraphQLGen_BUILD_DIR}/src/GraphQLGen-build/include/")

set(DEPS_INCLUDE_DIRS "${DEPS_INCLUDE_DIRS};${RapidJSON_INCLUDE_DIR};${GraphQLGen_INCLUDE_DIR}")

set(DEPS_INCLUDE_DIRS "${DEPS_INCLUDE_DIRS}" PARENT_SCOPE)
set(GraphQLGen_BINARY "${GraphQLGen_BUILD_DIR}/src/GraphQLGen-build/src/schemagen" PARENT_SCOPE)