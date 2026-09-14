#pragma once

#include "IronHull/render/RenderPass.hpp"
#include "raylib.h"
#include <string>

namespace IronHull
{
    struct Window
    {
        int width = 0;
        int height = 0;
        std::string title = "";
        bool resizable = 0;
        bool fullscreen = 0;
        int target_fps = 0;
    };

    struct Viewport
    {
        int width = 0;
        int height = 0;
    };

    struct Physics2DSettings
    {
        Vector2 gravity = { 0.0f, 1.0f }; 
        float pixels_per_unit = 32.0f;
    };

    class Application
    {
        private:
            static Application* instance;
        private:
            bool is_running;
            RenderTexture2D target;
        protected:
            Window window;
            Viewport viewport;
            Physics2DSettings physics_2d;
            std::string project_name;
        public:
            static void quit();
            static bool is_debug();
        public:
            static Window get_window();
            static Viewport get_viewport();
            static Physics2DSettings get_physics2d();
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
            void frame();
#if defined(PLATFORM_WEB)
            static void web_frame(void* arg);
#endif
        protected:
            virtual void on_compose() { }
            virtual void on_ready() { }
            virtual void on_update(float delta) { } 
            virtual void on_draw(RenderPass pass) { }
            virtual void on_dispose() { }
    };

    Application* create_application();
}
