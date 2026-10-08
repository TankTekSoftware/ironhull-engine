#include <Rivet/popup/NewProjectPopup.hpp>

namespace Rivet
{
    NewProjectPopup::NewProjectPopup() : EditorPopup("New Project"), project_location("Project Location", FileInputMode::MODE_FOLDER)
    {
        this->initialize();
    }

    void NewProjectPopup::initialize()
    {
        this->project_name[0] = '\0';
        this->project_location.clear();
        this->project_template = ProjectTemplate::TEMPLATE_EMPTY;
    }

    bool NewProjectPopup::is_valid() const
    {
        return this->project_name[0] != '\0' && !this->project_location.is_empty();
    }

    void NewProjectPopup::create_project()
    {

    }

    void NewProjectPopup::on_open()
    {
        this->initialize();
    }

    void NewProjectPopup::on_draw()
    {
        ImGui::InputText("Project Name", this->project_name, IM_ARRAYSIZE(this->project_name));

        this->project_location.draw();

        this->draw_project_templates();
        this->draw_action_buttons();
    }

    void NewProjectPopup::draw_project_templates()
    {
        ImGui::Text("Project Template");
        if (ImGui::RadioButton("Empty", this->project_template == ProjectTemplate::TEMPLATE_EMPTY)) {
            this->project_template = ProjectTemplate::TEMPLATE_EMPTY;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("First Person", this->project_template == ProjectTemplate::TEMPLATE_FIRST_PERSON)) {
            this->project_template = ProjectTemplate::TEMPLATE_FIRST_PERSON;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Third Person", this->project_template == ProjectTemplate::TEMPLATE_THIRD_PERSON)) {
            this->project_template = ProjectTemplate::TEMPLATE_THIRD_PERSON;
        }
    }

    void NewProjectPopup::draw_action_buttons()
    {
        ImGui::BeginDisabled(!this->is_valid());
        if (ImGui::Button("Create")) {
            // TODO: Create a new project.
            this->close();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        if (ImGui::Button("Cancel")) {
            this->close();
        }
    }
}