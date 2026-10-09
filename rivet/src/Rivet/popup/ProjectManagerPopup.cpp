#include <Rivet/popup/ProjectManagerPopup.hpp>

namespace Rivet
{
    ProjectManagerPopup::ProjectManagerPopup() : EditorPopup("Rivet Project Manager")
    {
        this->project_location = new ImGuiFileInput("Project Location", FileInputMode::MODE_FOLDER);
        this->default_options();
    }

    ProjectManagerPopup::~ProjectManagerPopup()
    {
        delete this->project_location;
    }

    void ProjectManagerPopup::default_options()
    {
        this->project_name[0] = '\0';
        this->project_location->clear();
        this->project_template = ProjectTemplate::TEMPLATE_EMPTY;
    }

    bool ProjectManagerPopup::is_valid() const
    {
        return this->project_name[0] != '\0' && !this->project_location->is_empty();
    }

    void ProjectManagerPopup::create_project()
    {

    }

    void ProjectManagerPopup::on_open()
    {
        this->default_options();
    }

    void ProjectManagerPopup::on_draw()
    {
        ImGui::InputText("Project Name", this->project_name, IM_ARRAYSIZE(this->project_name));

        this->project_location->draw();

        this->draw_project_templates();
        this->draw_action_buttons();
    }

    void ProjectManagerPopup::draw_project_templates()
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

    void ProjectManagerPopup::draw_action_buttons()
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