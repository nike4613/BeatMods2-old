


add_custom_command(
	OUTPUT "generated/Schema.h"
    OUTPUT "generated/Schema.cpp"
    
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated/"
    COMMAND ${GraphQLGen_BINARY} -s "${CMAKE_CURRENT_SOURCE_DIR}/api.graphql" -p "${CMAKE_CURRENT_BINARY_DIR}/generated/" -n BeatMods --no-stubs -v

    DEPENDS GraphQLGen "${CMAKE_CURRENT_SOURCE_DIR}/api.graphql"
)

