cmake_minimum_required(VERSION 3.25)
project(IronHullEngine LANGUAGES CXX C)

# --- Compile Commands Settings --- #
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(COMPILE_COMMANDS_TOP ${CMAKE_SOURCE_DIR}/compile_commands.json)

if(NOT TARGET export_compile_commands_top)
    add_custom_target(export_compile_commands_top ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${COMPILE_COMMANDS_TOP}
        COMMENT "Exporting compile_commands.json to top level"
        VERBATIM
    )
endif()

# --- Configure C++ --- #
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
endif()

# --- Configure Output Directory --- #
string(TOLOWER "${PLATFORM}" PLATFORM_DIR)
string(TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${PLATFORM_DIR}-${BUILD_TYPE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${PLATFORM_DIR}-${BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${PLATFORM_DIR}-${BUILD_TYPE})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/engine/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/sandbox/)
