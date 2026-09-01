#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace IronHull
{
    // Resolves protocol-prefixed asset paths onto disk and installs raylib's custom file IO
    // callbacks so raylib's own Load*()/Save*() functions can read and write through it
    // transparently.
    //
    //   assets://path  Read-only, packaged with the app: a loose "assets/" directory next
    //                   to the executable in debug builds, or a PhysFS-mounted "assets.ihpk"
    //                   archive next to the executable in release (NDEBUG) builds.
    //
    //   user://path     Writable, per-user application data directory:
    //                     Windows: %APPDATA%/<project_name>/
    //                     macOS:   ~/Library/Application Support/<project_name>/
    //                     Linux:   ~/.local/share/<project_name>/
    //
    // NOTE: once initialized, ALL raylib file IO (LoadTexture, LoadShader, LoadModel, SaveFileData,
    // ...) is routed through this filesystem, so any path passed anywhere in the engine must use
    // one of the two protocols above - bare filesystem paths will no longer resolve.
    class FileSystem
    {
        private:
            FileSystem() = default;

        public:
            static void init(const std::string& project_name);
            static void shutdown();

        public:
            static bool exists(const std::string& uri);
            static std::vector<unsigned char> read_bytes(const std::string& uri);
            static bool write_bytes(const std::string& uri, const void* data, size_t size);

        private:
            static std::string user_data_directory(const std::string& project_name);

        private:
            static unsigned char* on_load_file_data(const char* fileName, int* dataSize);
            static bool on_save_file_data(const char* fileName, void* data, int dataSize);
            static char* on_load_file_text(const char* fileName);
            static bool on_save_file_text(const char* fileName, const char* text);
    };
}
