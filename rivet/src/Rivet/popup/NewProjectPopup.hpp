#pragma once

#include <Rivet/popup/EditorPopup.hpp>
#include <Rivet/utils/ImGuiFileInput.hpp>

namespace Rivet
{
    enum class ProjectTemplate
    {
        TEMPLATE_EMPTY,
        TEMPLATE_FIRST_PERSON,
        TEMPLATE_THIRD_PERSON,
    };

    class NewProjectPopup : public Rivet::EditorPopup
    {
        private:
            char project_name[128];
            ImGuiFileInput project_location;
            ProjectTemplate project_template;
        public:
            NewProjectPopup();
        private:
            void initialize();
            bool is_valid() const;
            void create_project();
        protected:
            void on_open() override;
            void on_draw() override;
        private:
            void draw_project_templates();
            void draw_action_buttons();
    };
}
