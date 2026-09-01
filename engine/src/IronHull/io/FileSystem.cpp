#include <IronHull/io/FileSystem.hpp>

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include <physfs.h>
#include <raylib.h>

namespace IronHull
{
    namespace
    {
        // Backing store for "assets://": a loose "assets/" directory in debug builds, or an
        // "assets.ihpk" archive in release (NDEBUG) builds, mounted at PhysFS's virtual root
        // by init(). PhysFS auto-detects the archive format, so the same mount/read calls
        // work for both cases.
        bool g_assets_mounted = false;

        // Backing store for "user://": a plain writable directory on disk.
        std::string g_user_dir;

        enum class Protocol
        {
            Assets,
            User,
        };

        struct ParsedUri
        {
            Protocol protocol;
            std::string relative_path; // protocol stripped, forward-slash separated
        };

        ParsedUri parse_uri(const std::string& uri)
        {
            static const std::string assets_protocol = "assets://";
            static const std::string user_protocol = "user://";

            if (uri.compare(0, assets_protocol.size(), assets_protocol) == 0) {
                return { Protocol::Assets, uri.substr(assets_protocol.size()) };
            }

            if (uri.compare(0, user_protocol.size(), user_protocol) == 0) {
                return { Protocol::User, uri.substr(user_protocol.size()) };
            }

            throw std::invalid_argument("FileSystem: path '" + uri + "' does not use a recognized protocol (expected 'assets://' or 'user://')");
        }

        std::string user_disk_path(const std::string& relative_path)
        {
            return g_user_dir + "/" + relative_path;
        }

        bool read_disk_file(const std::string& path, std::vector<unsigned char>& out)
        {
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                return false;
            }

            std::streamoff length = file.tellg();
            if (length < 0) {
                return false;
            }

            out.resize(static_cast<size_t>(length));

            if (length > 0) {
                file.seekg(0);
                file.read(reinterpret_cast<char*>(out.data()), length);
                if (!file) {
                    return false;
                }
            }

            return true;
        }

        std::vector<unsigned char> read_assets_file(const std::string& uri, const std::string& relative_path)
        {
            PHYSFS_File* file = PHYSFS_openRead(relative_path.c_str());
            if (file == nullptr) {
                throw std::runtime_error("FileSystem: failed to read '" + uri + "' from assets: " +
                    PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
            }

            PHYSFS_sint64 length = PHYSFS_fileLength(file);
            if (length < 0) {
                PHYSFS_close(file);
                throw std::runtime_error("FileSystem: failed to determine length of '" + uri + "'");
            }

            std::vector<unsigned char> data(static_cast<size_t>(length));

            if (length > 0) {
                PHYSFS_sint64 read = PHYSFS_readBytes(file, data.data(), static_cast<PHYSFS_uint64>(length));
                if (read != length) {
                    std::string error = PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
                    PHYSFS_close(file);
                    throw std::runtime_error("FileSystem: failed to read '" + uri + "' from assets: " + error);
                }
            }

            PHYSFS_close(file);
            return data;
        }
    }

    void FileSystem::init(const std::string& project_name)
    {
        if (!PHYSFS_init(nullptr)) {
            TraceLog(LOG_WARNING, "FILESYSTEM: Failed to initialize PhysFS: %s",
                PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
        }

        std::string app_dir = GetApplicationDirectory();

#ifdef NDEBUG
        std::string assets_source = app_dir + "assets.ihpk";
#else
        std::string assets_source = app_dir + "assets";
#endif

        if (PHYSFS_mount(assets_source.c_str(), "/", 1)) {
            g_assets_mounted = true;
        } else {
            TraceLog(LOG_WARNING, "FILESYSTEM: Failed to mount assets '%s': %s", assets_source.c_str(),
                PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
        }

        g_user_dir = FileSystem::user_data_directory(project_name);

        std::error_code ec;
        std::filesystem::create_directories(g_user_dir, ec);

        if (ec) {
            TraceLog(LOG_WARNING, "FILESYSTEM: Failed to create user directory '%s': %s", g_user_dir.c_str(), ec.message().c_str());
        }

        SetLoadFileDataCallback(FileSystem::on_load_file_data);
        SetSaveFileDataCallback(FileSystem::on_save_file_data);
        SetLoadFileTextCallback(FileSystem::on_load_file_text);
        SetSaveFileTextCallback(FileSystem::on_save_file_text);
    }

    void FileSystem::shutdown()
    {
        SetLoadFileDataCallback(nullptr);
        SetSaveFileDataCallback(nullptr);
        SetLoadFileTextCallback(nullptr);
        SetSaveFileTextCallback(nullptr);

        PHYSFS_deinit();
        g_assets_mounted = false;

        g_user_dir.clear();
    }

    std::string FileSystem::user_data_directory(const std::string& project_name)
    {
#if defined(_WIN32)
        const char* base = std::getenv("APPDATA");
        std::string root = base != nullptr ? base : ".";
#elif defined(__APPLE__)
        const char* home = std::getenv("HOME");
        std::string root = std::string(home != nullptr ? home : ".") + "/Library/Application Support";
#else
        const char* home = std::getenv("HOME");
        std::string root = std::string(home != nullptr ? home : ".") + "/.local/share";
#endif

        return root + "/" + project_name;
    }

    bool FileSystem::exists(const std::string& uri)
    {
        ParsedUri parsed = parse_uri(uri);

        if (parsed.protocol == Protocol::Assets) {
            return g_assets_mounted && PHYSFS_exists(parsed.relative_path.c_str()) != 0;
        }

        std::error_code ec;
        return std::filesystem::exists(user_disk_path(parsed.relative_path), ec);
    }

    std::vector<unsigned char> FileSystem::read_bytes(const std::string& uri)
    {
        ParsedUri parsed = parse_uri(uri);

        if (parsed.protocol == Protocol::Assets) {
            if (!g_assets_mounted) {
                throw std::runtime_error("FileSystem: failed to read '" + uri + "': assets are not mounted");
            }

            return read_assets_file(uri, parsed.relative_path);
        }

        std::vector<unsigned char> data;
        if (!read_disk_file(user_disk_path(parsed.relative_path), data)) {
            throw std::runtime_error("FileSystem: failed to read '" + uri + "'");
        }
        return data;
    }

    bool FileSystem::write_bytes(const std::string& uri, const void* data, size_t size)
    {
        ParsedUri parsed = parse_uri(uri);

        if (parsed.protocol != Protocol::User) {
            throw std::runtime_error("FileSystem: '" + uri + "' is read-only (only 'user://' paths can be written)");
        }

        std::string path = user_disk_path(parsed.relative_path);

        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);

        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            throw std::runtime_error("FileSystem: failed to open '" + uri + "' for writing");
        }

        if (size > 0) {
            file.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
        }

        if (!file) {
            throw std::runtime_error("FileSystem: failed to fully write '" + uri + "'");
        }

        return true;
    }

    // raylib's custom file IO callbacks are plain C function pointers invoked from inside
    // raylib's own C sources, so exceptions must never cross this boundary - errors are
    // logged and reported back using raylib's normal null/false "failed to load" contract.
    // Buffers are allocated with malloc() since raylib frees them with its default allocator
    // (RL_FREE / free()) via UnloadFileData()/UnloadFileText().

    unsigned char* FileSystem::on_load_file_data(const char* fileName, int* dataSize)
    {
        if (dataSize != nullptr) {
            *dataSize = 0;
        }

        if (fileName == nullptr) {
            return nullptr;
        }

        try {
            std::vector<unsigned char> bytes = FileSystem::read_bytes(fileName);

            unsigned char* data = static_cast<unsigned char*>(std::malloc(bytes.size()));
            if (data == nullptr) {
                return nullptr;
            }

            if (!bytes.empty()) {
                std::memcpy(data, bytes.data(), bytes.size());
            }

            if (dataSize != nullptr) {
                *dataSize = static_cast<int>(bytes.size());
            }

            return data;
        } catch (const std::exception& error) {
            TraceLog(LOG_WARNING, "FILESYSTEM: %s", error.what());
            return nullptr;
        }
    }

    bool FileSystem::on_save_file_data(const char* fileName, void* data, int dataSize)
    {
        if (fileName == nullptr || dataSize < 0) {
            return false;
        }

        try {
            return FileSystem::write_bytes(fileName, data, static_cast<size_t>(dataSize));
        } catch (const std::exception& error) {
            TraceLog(LOG_WARNING, "FILESYSTEM: %s", error.what());
            return false;
        }
    }

    char* FileSystem::on_load_file_text(const char* fileName)
    {
        if (fileName == nullptr) {
            return nullptr;
        }

        try {
            std::vector<unsigned char> bytes = FileSystem::read_bytes(fileName);

            char* text = static_cast<char*>(std::malloc(bytes.size() + 1));
            if (text == nullptr) {
                return nullptr;
            }

            if (!bytes.empty()) {
                std::memcpy(text, bytes.data(), bytes.size());
            }
            text[bytes.size()] = '\0';

            return text;
        } catch (const std::exception& error) {
            TraceLog(LOG_WARNING, "FILESYSTEM: %s", error.what());
            return nullptr;
        }
    }

    bool FileSystem::on_save_file_text(const char* fileName, const char* text)
    {
        if (fileName == nullptr || text == nullptr) {
            return false;
        }

        try {
            return FileSystem::write_bytes(fileName, text, std::strlen(text));
        } catch (const std::exception& error) {
            TraceLog(LOG_WARNING, "FILESYSTEM: %s", error.what());
            return false;
        }
    }
}
