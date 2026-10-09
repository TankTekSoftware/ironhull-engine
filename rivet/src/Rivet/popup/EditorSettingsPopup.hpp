#pragma once

#include <Rivet/popup/EditorPopup.hpp>

namespace Rivet
{
    enum class EditorSettingsCategory
    {
        CATEGORY_THEME,
        CATEGORY_TEXT_EDITOR,
    };

    enum class EditorTheme
    {
        THEME_DEFAULT,
        THEME_LIGHT,
        THEME_CLASSIC,
        THEME_SRC,
    };

    class EditorSettingsPopup : public Rivet::EditorPopup
    {
        private:
            EditorSettingsCategory category;

            // --- THEME --- //
            EditorTheme theme;
            float ui_scale;
            float window_rounding;
            float frame_rounding;

            // --- TEXT EDITOR --- //
            int font_size;
            int tab_size;
            bool insert_spaces;
            bool show_line_numbers;
            bool word_wrap;
            bool highlight_current_line;
        public:
            EditorSettingsPopup();
            void apply_theme() const;
        private:
            void default_theme_settings();
            void default_text_editor_settings();
        protected:
            void on_draw() override;
        private:
            void draw_categories();
            void draw_category_settings();
            void draw_theme_settings();
            void draw_text_editor_settings();
            void draw_action_buttons();
    };
}
