#pragma once

#include "IronHull/render/RenderPass.hpp"
#include <IronHull/scene/Scene.hpp>
#include <string>
#include <unordered_map>

namespace IronHull
{
    class SceneManager
    {
        private:
            std::unordered_map<std::string, Scene*> scenes;
            std::string current_key;
            Scene* current_scene = nullptr;

        private:
            static SceneManager& get_singleton();

        private:
            SceneManager() = default;
            ~SceneManager();
            SceneManager(const SceneManager&) = delete;
            SceneManager& operator=(const SceneManager&) = delete;

        public:
            template <typename T>
            static void register_scene(const std::string& key)
            {
                SceneManager::register_scene(key, new T());
            }

            static void register_scene(const std::string& key, Scene* scene);
        
        public:
            static void change_scene(const std::string& key);
        
        public:
            static void ready();
            static void update(float delta);
            static void draw(RenderPass pass);
            static void dispose();
    };
}
