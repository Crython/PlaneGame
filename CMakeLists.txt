cmake_minimum_required(VERSION 3.16)

project(PlaneGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

list(FILTER PROJECT_SOURCES EXCLUDE REGEX "^${CMAKE_CURRENT_BINARY_DIR}/")

if(NOT PROJECT_SOURCES)
    message(FATAL_ERROR "No .cpp source files found in ${CMAKE_CURRENT_SOURCE_DIR}/src/")
endif()

add_executable(PlaneGame ${PROJECT_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/include/tracy-0.14.1/public/TracyClient.cpp )

find_package(SFML 3 COMPONENTS Graphics REQUIRED)
target_link_libraries(PlaneGame PRIVATE SFML::Graphics)
target_compile_definitions(PlaneGame PRIVATE
    PLANE_GAME_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

# Allow #include <SFML/...> instead of #include "../include/SFML/..."
target_include_directories(PlaneGame PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_definitions(PlaneGame PRIVATE TRACY_ENABLE)

if(APPLE)
    find_program(DSYMUTIL_EXECUTABLE dsymutil)
    if(NOT DSYMUTIL_EXECUTABLE)
        message(FATAL_ERROR "dsymutil is required to generate debug symbols for Tracy callstacks")
    endif()

    add_custom_command(TARGET PlaneGame POST_BUILD
        COMMAND "${DSYMUTIL_EXECUTABLE}" "$<TARGET_FILE:PlaneGame>"
        COMMENT "Generating PlaneGame.dSYM"
        VERBATIM
    )
endif()
