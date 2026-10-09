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
            ImVec2 initial_size;
            ImVec2 min_size;
        private:
            bool visible;
            bool open_requested;
            bool active;
        public:
            // A zero initial_size auto-fits the popup to its content the first time it opens.
            EditorPopup(
                const std::string& title,
                const ImVec2& initial_size = ImVec2(0.0f, 0.0f),
                const ImVec2& min_size = ImVec2(0.0f, 0.0f),
                ImGuiWindowFlags flags = ImGuiWindowFlags_None
            );
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
