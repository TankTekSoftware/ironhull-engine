#pragma once

#include <any>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include <raylib.h>

#include <IronHull/asset/ShaderPreprocessor.hpp>
#include <IronHull/io/FileSystem.hpp>

namespace IronHull
{
    // Loads/unloads a raylib asset type. `path` is a assets://... or user://... URI (see
    // FileSystem). `keep_alive` is an out param a loader may populate to keep extra data
    // alive for as long as the asset lives in the registry; most loaders leave it untouched.
    template <typename T>
    struct AssetLoader
    {
        static_assert(sizeof(T) == 0, "AssetLoader<T> has no specialization for this type. Specialize IronHull::AssetLoader<T> with static load(const std::string&, std::any&)/unload(T&) functions to register a new asset type.");
    };

    // Texture2D/Image/Font/Model load through raylib's own path-based Load*() functions,
    // which transparently resolve assets://... and user://... URIs via the raylib file IO
    // callbacks FileSystem installs.

    template <>
    struct AssetLoader<Texture2D>
    {
        // Bilinear filtering by default so rotated/scaled sprites (e.g. a wobbling title
        // logo) sample smoothly instead of showing hard, aliased texel edges.
        static Texture2D load(const std::string& path, std::any&)
        {
            Texture2D texture = LoadTexture(path.c_str());
            SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
            return texture;
        }

        static void unload(Texture2D& asset) { UnloadTexture(asset); }
    };

    template <>
    struct AssetLoader<Image>
    {
        static Image load(const std::string& path, std::any&) { return LoadImage(path.c_str()); }
        static void unload(Image& asset) { UnloadImage(asset); }
    };

    template <>
    struct AssetLoader<Font>
    {
        static Font load(const std::string& path, std::any&) { return LoadFontEx(path.c_str(), 256, NULL, 0); }
        static void unload(Font& asset) { UnloadFont(asset); }
    };

    template <>
    struct AssetLoader<Model>
    {
        static Model load(const std::string& path, std::any&) { return LoadModel(path.c_str()); }
        static void unload(Model& asset) { UnloadModel(asset); }
    };

    // Shader source is read as text and split by ShaderPreprocessor on `#type vertex` /
    // `#type fragment` markers, so a single .glsl file can carry either stage or both. A
    // missing vertex stage passes null through to raylib, which falls back to its default
    // vertex shader - all a purely fragment-driven effect needs.
    template <>
    struct AssetLoader<Shader>
    {
        static Shader load(const std::string& path, std::any&)
        {
            std::vector<unsigned char> bytes = FileSystem::read_bytes(path);
            std::string source(bytes.begin(), bytes.end());

            ShaderPreprocessor::Stages stages = ShaderPreprocessor::split(source);
            if (!stages.has_fragment) {
                throw std::invalid_argument("AssetRegistry: shader '" + path + "' has no '#type fragment' block");
            }

            const char* vertex_code = stages.has_vertex ? stages.vertex.c_str() : nullptr;
            return LoadShaderFromMemory(vertex_code, stages.fragment.c_str());
        }

        static void unload(Shader& asset) { UnloadShader(asset); }
    };

    // Sound and Music are special: raylib's audio module (raudio.c) reads files itself with
    // its own private fopen()-based IO and never goes through raylib's custom file IO
    // callbacks, so a assets://... or user://... URI has to be read via FileSystem
    // ourselves and handed to raylib as an in-memory buffer instead.

    template <>
    struct AssetLoader<Sound>
    {
        static Sound load(const std::string& path, std::any&)
        {
            std::vector<unsigned char> bytes = FileSystem::read_bytes(path);
            Wave wave = LoadWaveFromMemory(GetFileExtension(path.c_str()), bytes.data(), static_cast<int>(bytes.size()));
            Sound sound = LoadSoundFromWave(wave);
            UnloadWave(wave);
            return sound;
        }

        static void unload(Sound& asset) { UnloadSound(asset); }
    };

    template <>
    struct AssetLoader<Music>
    {
        // Unlike LoadSoundFromWave, raylib's WAV/OGG/MP3 memory decoders keep referencing the
        // source buffer for the lifetime of the stream instead of copying it up front, so the
        // bytes are handed back through `keep_alive` and only freed when the asset is unloaded.
        static Music load(const std::string& path, std::any& keep_alive)
        {
            auto bytes = std::make_shared<std::vector<unsigned char>>(FileSystem::read_bytes(path));
            Music music = LoadMusicStreamFromMemory(GetFileExtension(path.c_str()), bytes->data(), static_cast<int>(bytes->size()));
            keep_alive = bytes;
            return music;
        }

        static void unload(Music& asset) { UnloadMusicStream(asset); }
    };

    class AssetRegistry
    {
        private:
            struct Entry
            {
                std::any data;
                std::any keep_alive;
                std::type_index type = std::type_index(typeid(void));
                std::function<void(std::any&)> unload;
            };

        private:
            std::unordered_map<std::string, Entry> assets;

        private:
            static AssetRegistry& get_singleton();

        private:
            AssetRegistry() = default;
            ~AssetRegistry() = default;
            AssetRegistry(const AssetRegistry&) = delete;
            AssetRegistry& operator=(const AssetRegistry&) = delete;

        public:
            template <typename T>
            static void load(const std::string& key, const std::string& path)
            {
                AssetRegistry& self = AssetRegistry::get_singleton();

                auto it = self.assets.find(key);
                if (it != self.assets.end()) {
                    if (it->second.type != std::type_index(typeid(T))) {
                        throw std::invalid_argument("AssetRegistry: asset '" + key + "' is already registered as a different type");
                    }

                    return;
                }

                Entry entry;
                entry.data = AssetLoader<T>::load(path, entry.keep_alive);
                entry.type = std::type_index(typeid(T));
                entry.unload = [](std::any& data) {
                    T& asset = std::any_cast<T&>(data);
                    AssetLoader<T>::unload(asset);
                };

                self.assets.emplace(key, std::move(entry));
            }

            template <typename T>
            static T& get(const std::string& key)
            {
                AssetRegistry& self = AssetRegistry::get_singleton();

                auto it = self.assets.find(key);
                if (it == self.assets.end()) {
                    throw std::invalid_argument("AssetRegistry: no asset registered under key '" + key + "'");
                }

                if (it->second.type != std::type_index(typeid(T))) {
                    throw std::invalid_argument("AssetRegistry: asset '" + key + "' is not of the requested type");
                }

                return std::any_cast<T&>(it->second.data);
            }

        public:
            static bool has(const std::string& key);
            static void unload(const std::string& key);

        public:
            static void dispose();
    };
}
