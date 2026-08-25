#include "IronHull/render/RenderPass.hpp"
#include <IronHull/scene/SceneManager.hpp>

#include <stdexcept>

namespace IronHull
{
    SceneManager& SceneManager::get_singleton()
    {
        static SceneManager singleton;
        return singleton;
    }

    SceneManager::~SceneManager()
    {
        for (auto& [key, scene] : this->scenes) {
            delete scene;
        }

        this->scenes.clear();
        this->current_scene = nullptr;
    }

    void SceneManager::register_scene(const std::string& key, Scene* scene)
    {
        SceneManager& self = SceneManager::get_singleton();

        if (self.scenes.find(key) != self.scenes.end()) {
            delete scene;
            throw std::invalid_argument("SceneManager: a scene is already registered under key '" + key + "'");
        }

        self.scenes[key] = scene;
    }

    void SceneManager::change_scene(const std::string& key)
    {
        SceneManager& self = SceneManager::get_singleton();

        auto it = self.scenes.find(key);
        if (it == self.scenes.end()) {
            throw std::invalid_argument("SceneManager: no scene registered under key '" + key + "'");
        }

        if (self.current_scene != nullptr) {
            self.current_scene->dispose();
        }

        self.current_key = key;
        self.current_scene = it->second;
        self.current_scene->ready();
    }

    void SceneManager::ready()
    {
        SceneManager& self = SceneManager::get_singleton();

        if (self.current_scene != nullptr) {
            self.current_scene->ready();
        }
    }

    void SceneManager::update(float delta)
    {
        SceneManager& self = SceneManager::get_singleton();

        if (self.current_scene != nullptr) {
            self.current_scene->update(delta);
        }
    }

    void SceneManager::draw(RenderPass pass)
    {
        SceneManager& self = SceneManager::get_singleton();

        if (self.current_scene != nullptr) {
            self.current_scene->draw(pass);
        }
    }

    void SceneManager::dispose()
    {
        SceneManager& self = SceneManager::get_singleton();

        if (self.current_scene != nullptr) {
            self.current_scene->dispose();
            self.current_scene = nullptr;
        }

        self.current_key.clear();
    }

}
