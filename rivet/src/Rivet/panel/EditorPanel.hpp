#pragma once

#include <string>

#include <imgui.h>

namespace Rivet
{
    class EditorPanel
    {
        protected:
            std::string title;
            ImGuiWindowFlags flags;
        public:
            EditorPanel(const std::string& title, ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse);
            virtual ~EditorPanel();
        protected:
            virtual void on_draw() = 0;
        public:
            void draw();
    };
}