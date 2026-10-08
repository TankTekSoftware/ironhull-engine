#include <Rivet/panel/EntitiesPanel.hpp>

#include <imgui.h>

namespace Rivet
{

    EntitiesPanel::EntitiesPanel() : EditorPanel("Entities") 
    {
        
    }

    void EntitiesPanel::on_draw()
    {
        ImGui::Text("TODO: Implement Entities Panel.");
    }
}