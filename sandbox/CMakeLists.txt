cmake_minimum_required(VERSION 3.25)
project(SandboxApp)


file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        IronHullFramework
)

# Emscripten executables default to a bare ".js" glue script; ".html" gets emcc to also
# generate a minimal page with a <canvas> that loads it, which is what's actually meant to
# be opened/served.
if(PLATFORM STREQUAL "Web")
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")

    target_link_options(${PROJECT_NAME} PRIVATE
        -sALLOW_MEMORY_GROWTH=1
    )
endif()
