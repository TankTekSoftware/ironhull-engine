#pragma once

#include <functional>

namespace Rivet
{
    class MenuBar
    {
        private:
            std::function<void()> open_new_project;
            std::function<void()> open_project_settings;
        public:
            MenuBar();
            ~MenuBar();
        public:
            void draw();
        public:
            void set_open_new_project(const std::function<void()> callback);
            void set_open_project_settings(const std::function<void()> callback);
    };
}

