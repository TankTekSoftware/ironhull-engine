cmake_minimum_required(VERSION 3.25)
project(lua)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

# lua.c (the standalone interpreter) and luac.c (the bytecode compiler) each
# carry their own main(); they are programs shipped alongside the library, not
# part of it.
list(REMOVE_ITEM SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lua.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/luac.c
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/
)

# Lua is C99 and probes the host for its own feature set.
set_target_properties(${PROJECT_NAME} PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)

if(UNIX AND NOT APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE LUA_USE_LINUX)
elseif(APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE LUA_USE_MACOSX)
endif()