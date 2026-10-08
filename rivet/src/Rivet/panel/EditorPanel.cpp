#include <Rivet/panel/EditorPanel.hpp>

namespace Rivet
{
    EditorPanel::EditorPanel(const std::string& title, ImGuiWindowFlags flags)
    {
        this->title = title;
        this->flags = flags;
    }

    EditorPanel::~EditorPanel()
    {

    }

    void EditorPanel::draw()
    {
        ImGui::Begin(this->title.c_str()); 
        {
            this->on_draw();
        }   
        ImGui::End();
    }
}