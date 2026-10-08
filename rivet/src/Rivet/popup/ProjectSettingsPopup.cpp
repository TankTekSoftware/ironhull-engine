#include <Rivet/popup/ProjectSettingsPopup.hpp>

namespace Rivet
{
    ProjectSettingsPopup::ProjectSettingsPopup() : EditorPopup("Project Settings")
    {
        this->selected_category = Category::GENERAL;
    }

    const char* ProjectSettingsPopup::get_category_name(Category category)
    {
        switch (category) {
            case Category::GENERAL:   return "General";
            case Category::DISPLAY:   return "Display";
            case Category::INPUT:     return "Input";
            default:                  return "Unknown";
        }
    }

    void ProjectSettingsPopup::on_draw() 
    {
        this->draw_category_navigation();

        ImGui::SameLine();

        this->draw_category_context();

        // On click ProjectSettings will close WITH saving.
        if (ImGui::Button("Save")) {
            // TODO: Write project settings.
            this->close();
        }

        ImGui::SameLine();

        // On click ProjectSettings will close WITHOUT save.
        if (ImGui::Button("Cancel")) {
            this->close();
        }
    }

    void ProjectSettingsPopup::draw_category_navigation()
    {
        if (ImGui::BeginChild("##CategoryNavigation", ImVec2(150.0f, 300.0f), ImGuiChildFlags_Borders)) {
            for (int i = 0; i < static_cast<int>(Category::COUNT); i++) {
                Category category = static_cast<Category>(i);

                if (ImGui::Selectable(get_category_name(category), this->selected_category == category)) {
                    this->selected_category = category;
                }
            }
        }
        ImGui::EndChild();
    }

    void ProjectSettingsPopup::draw_category_context()
    {
        if (ImGui::BeginChild("##CategoryContext", ImVec2(400.0f, 300.0f), ImGuiChildFlags_Borders)) {
            ImGui::SeparatorText(get_category_name(this->selected_category));

            switch (this->selected_category) {
                case Category::GENERAL:
                    // TODO: Render general settings.
                    break;
                case Category::DISPLAY:
                    // TODO: Render display settings.
                    break;
                case Category::INPUT:
                    // TODO: Render input settings.
                    break;
                default:
                    break;
            }
        }
        ImGui::EndChild();
    }

}
