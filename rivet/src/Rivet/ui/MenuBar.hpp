#pragma once

#include <functional>

namespace Rivet
{
    class MenuBar
    {
        private:
            std::function<void()> on_open_editor_settings;
        public:
            MenuBar();
            ~MenuBar();
        public:
            void draw();
        private:
            void draw_map_menu();
            void draw_project_menu();
            void draw_debug_menu();
            void draw_editor_menu();
        public:
            void set_open_editor_settings(const std::function<void()> callback);
    };
}

