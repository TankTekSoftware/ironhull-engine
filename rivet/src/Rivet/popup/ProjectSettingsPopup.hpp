#pragma once

#include <Rivet/popup/EditorPopup.hpp>

namespace Rivet
{
    class ProjectSettingsPopup : public Rivet::EditorPopup
    {
        private:
            enum class Category
            {
                GENERAL,
                DISPLAY,
                INPUT,
                COUNT
            };

            static const char* get_category_name(Category category);
        private:
            Category selected_category;
        public:
            ProjectSettingsPopup();
        protected:
            void on_draw() override;
        private:
            void draw_category_navigation();
            void draw_category_context();
    };
}
