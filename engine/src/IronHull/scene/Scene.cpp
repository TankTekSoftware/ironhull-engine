#include "IronHull/render/RenderPass.hpp"
#include <IronHull/scene/Scene.hpp>

namespace IronHull
{
    void Scene::ready()
    {
        this->on_ready();
    }

    void Scene::update(float delta)
    {
        this->on_update(delta);
    }

    void Scene::draw(RenderPass pass)
    {
        this->on_draw(pass);
    }

    void Scene::dispose()
    {
        this->on_dispose();
    }
}
