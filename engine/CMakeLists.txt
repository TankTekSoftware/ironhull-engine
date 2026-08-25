cmake_minimum_required(VERSION 3.25)
project(IronHullFramework)

# --- Fetch Raylib --- #
include(FetchContent)

message(STATUS "Fetching raylib...")
FetchContent_Declare(
    raylib
    GIT_REPOSITORY "https://github.com/raysan5/raylib.git"
    GIT_TAG "6.0"
)
FetchContent_MakeAvailable(raylib)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        raylib 
        imgui
)
