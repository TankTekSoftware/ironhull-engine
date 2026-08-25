#pragma once

#include "IronHull/render/RenderPass.hpp"
#include <corecrt.h>
#include <string>

namespace IronHull
{
    struct Window
    {
        int width;
        int height;
        std::string title;
        bool resizable;
        bool fullscreen;
        int target_fps;
    };

    struct Viewport
    {
        int width;
        int height;
    };

    class Application
    {
        private:
            static Application* instance;
        private:
            bool is_running;
            std::string project_name;
        protected:
            Window window;
            Viewport viewport;
        public:
            static void quit();
            static bool is_debug();
        public:
            static int get_window_width();
            static int get_window_height();
            static int get_viewport_width();
            static int get_viewport_height();
        public:
            Application() = default;
            ~Application() = default;
        public:
            void run();
        private:
            void compose();
            void ready();
            void update(float delta);
            void draw(RenderPass pass);
            void dispose();
        protected:
            virtual void on_compose() { }
            virtual void on_ready() { }
            virtual void on_update(float delta) { } 
            virtual void on_draw(RenderPass pass) { }
            virtual void on_dispose() { }
    };
}
