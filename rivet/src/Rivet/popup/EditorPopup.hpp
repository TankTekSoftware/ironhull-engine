#pragma once

#include <string>

#include <imgui.h>

namespace Rivet
{
    class EditorPopup
    {
        protected:
            std::string title;
            ImGuiWindowFlags flags;
        private:
            bool visible;
            bool open_requested;
            bool active;
        public:
            EditorPopup(const std::string& title, ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize);
            virtual ~EditorPopup();
        protected:
            virtual void on_draw() = 0;
            virtual void on_open();
            virtual void on_close();
        public:
            void draw();
            void open();
            void close();
            bool is_open() const;
        
    };
}
