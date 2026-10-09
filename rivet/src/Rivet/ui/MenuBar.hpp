#pragma once

#include <functional>

namespace Rivet
{
    class MenuBar
    {
        public:
            MenuBar();
            ~MenuBar();
        public:
            void draw();
        private:
            void draw_map_menu();
            void draw_project_menu();
            void draw_editor_menu();
    };
}

