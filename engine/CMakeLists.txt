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

message(STATUS "Fetcing physfs...")
set(PHYSFS_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(PHYSFS_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    physfs
    GIT_REPOSITORY "https://github.com/icculus/physfs.git"
    GIT_TAG "main"
)
FetchContent_MakeAvailable(physfs)

message(STATUS "Fetching box3d")
set(BOX3D_SAMPLES OFF CACHE BOOL "" FORCE)
set(BOX3D_TESTS OFF CACHE BOOL "" FORCE)
set(BOX3D_DOCS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    box3d
    GIT_REPOSITORY "https://github.com/erincatto/box3d.git"
    GIT_TAG "v0.1.0"
)
FetchContent_MakeAvailable(box3d)

message(STATUS "Fetching nfd...")
FetchContent_Declare(
    nfd
    GIT_REPOSITORY "https://github.com/btzy/nativefiledialog-extended.git"
    GIT_TAG "v1.3.0"
)
FetchContent_MakeAvailable(nfd)

message(STATUS "Fetching sol...")
FetchContent_Declare(
    sol
    GIT_REPOSITORY "https://github.com/ThePhD/sol2.git"
    GIT_TAG "v3.5.0"
)
FetchContent_MakeAvailable(sol)

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
        PhysFS::PhysFS-static
        box3d
        lua
        sol2
        nfd
)
