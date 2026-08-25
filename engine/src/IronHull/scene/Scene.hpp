#pragma once

#include "IronHull/render/RenderPass.hpp"
namespace IronHull
{
    class Scene
    {
        public:
            void ready();
            void update(float delta);
            void draw(RenderPass pass);
            void dispose();
        protected:
            virtual void on_ready() {}
            virtual void on_update(float delta) {}
            virtual void on_draw(RenderPass pass) {}
            virtual void on_dispose() {}
    };
}
