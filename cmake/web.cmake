# Toolchain file for building with Emscripten (emcc/em++).
#
# Just pointing CMAKE_C/CXX_COMPILER at emcc/em++ (as this file used to do) isn't enough:
# Emscripten ships its own CMake toolchain that also configures the sysroot, the
# ".js"/".wasm" executable suffix, find_*() search paths, etc. That file normally has to be
# passed explicitly (`emcmake cmake ..` or `-DCMAKE_TOOLCHAIN_FILE=.../Emscripten.cmake`), so
# this file locates it and defers to it, letting `cmake --preset=web-debug` do the same job
# on its own.
#
# Resolved via the EMSDK environment variable (set by sourcing emsdk_env.sh/.bat) when
# present, otherwise via the emcc/em++ binaries on PATH.
if(DEFINED ENV{EMSDK})
    set(IRONHULL_EMSDK_ROOT "$ENV{EMSDK}")
else()
    find_program(IRONHULL_EMCC_EXECUTABLE emcc)
    if(NOT IRONHULL_EMCC_EXECUTABLE)
        message(FATAL_ERROR
            "web.cmake: could not find 'emcc'. Install the Emscripten SDK "
            "(https://emscripten.org/docs/getting_started/downloads.html) and either add it "
            "to PATH or set the EMSDK environment variable (source emsdk_env.sh/.bat).")
    endif()

    # IRONHULL_EMCC_EXECUTABLE is normally "<emsdk>/upstream/emscripten/emcc".
    get_filename_component(IRONHULL_EMCC_DIR "${IRONHULL_EMCC_EXECUTABLE}" DIRECTORY)
    get_filename_component(IRONHULL_EMSDK_ROOT "${IRONHULL_EMCC_DIR}/../.." ABSOLUTE)
endif()

set(IRONHULL_EMSCRIPTEN_TOOLCHAIN "${IRONHULL_EMSDK_ROOT}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
if(NOT EXISTS "${IRONHULL_EMSCRIPTEN_TOOLCHAIN}")
    message(FATAL_ERROR
        "web.cmake: Emscripten toolchain file not found at '${IRONHULL_EMSCRIPTEN_TOOLCHAIN}'. "
        "Is EMSDK set to the right SDK root?")
endif()

include("${IRONHULL_EMSCRIPTEN_TOOLCHAIN}")
