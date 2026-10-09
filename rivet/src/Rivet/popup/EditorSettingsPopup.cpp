#include <Rivet/popup/EditorSettingsPopup.hpp>
#include <Rivet/theme/DefaultTheme.hpp>
#include <Rivet/theme/LightTheme.hpp>
#include <Rivet/theme/SrcTheme.hpp>

namespace Rivet
{
    static const float CATEGORY_PANEL_WIDTH = 180.0f;
    static const ImVec2 INITIAL_SIZE = ImVec2(640.0f, 440.0f);
    static const ImVec2 MIN_SIZE = ImVec2(480.0f, 300.0f);

    // Height for the panels so the action buttons below them stay visible.
    static float panel_height()
    {
        return -ImGui::GetFrameHeightWithSpacing();
    }

    EditorSettingsPopup::EditorSettingsPopup() : EditorPopup("Editor Settings", INITIAL_SIZE, MIN_SIZE)
    {
        this->category = EditorSettingsCategory::CATEGORY_THEME;
        this->default_theme_settings();
        this->default_text_editor_settings();
    }

    void EditorSettingsPopup::default_theme_settings()
    {
        this->theme = EditorTheme::THEME_DEFAULT;
        this->ui_scale = 1.0f;
        this->window_rounding = 0.0f;
        this->frame_rounding = 0.0f;
    }

    void EditorSettingsPopup::default_text_editor_settings()
    {
        this->font_size = 14;
        this->tab_size = 4;
        this->insert_spaces = true;
        this->show_line_numbers = true;
        this->word_wrap = false;
        this->highlight_current_line = true;
    }

    void EditorSettingsPopup::apply_theme() const
    {
        switch (this->theme) {
            case EditorTheme::THEME_DEFAULT: DefaultTheme::apply();        break;
            case EditorTheme::THEME_LIGHT:   LightTheme::apply();          break;
            case EditorTheme::THEME_CLASSIC: ImGui::StyleColorsClassic();  break;
            case EditorTheme::THEME_SRC:     SrcTheme::apply();            break;
        }

        ImGuiStyle& style = ImGui::GetStyle();
        style.FontScaleMain = this->ui_scale;
        style.WindowRounding = this->window_rounding;
        style.ChildRounding = this->window_rounding;
        style.PopupRounding = this->window_rounding;
        style.FrameRounding = this->frame_rounding;
        style.GrabRounding = this->frame_rounding;
    }

    void EditorSettingsPopup::on_draw()
    {
        this->draw_categories();
        ImGui::SameLine();
        this->draw_category_settings();

        this->draw_action_buttons();
    }

    void EditorSettingsPopup::draw_categories()
    {
        ImGui::BeginChild("##Categories", ImVec2(CATEGORY_PANEL_WIDTH, panel_height()), ImGuiChildFlags_Borders);

        if (ImGui::Selectable("Theme", this->category == EditorSettingsCategory::CATEGORY_THEME)) {
            this->category = EditorSettingsCategory::CATEGORY_THEME;
        }
        if (ImGui::Selectable("Text Editor", this->category == EditorSettingsCategory::CATEGORY_TEXT_EDITOR)) {
            this->category = EditorSettingsCategory::CATEGORY_TEXT_EDITOR;
        }

        ImGui::EndChild();
    }

    void EditorSettingsPopup::draw_category_settings()
    {
        ImGui::BeginChild("##Settings", ImVec2(0.0f, panel_height()), ImGuiChildFlags_Borders);

        switch (this->category) {
            case EditorSettingsCategory::CATEGORY_THEME:       this->draw_theme_settings();       break;
            case EditorSettingsCategory::CATEGORY_TEXT_EDITOR: this->draw_text_editor_settings(); break;
        }

        ImGui::EndChild();
    }

    void EditorSettingsPopup::draw_theme_settings()
    {
        ImGui::SeparatorText("Theme");

        bool changed = false;

        static const char* theme_names[] = { "Default", "Light", "Classic", "SRC" };
        int theme_index = (int)this->theme;
        if (ImGui::Combo("Color Theme", &theme_index, theme_names, IM_ARRAYSIZE(theme_names))) {
            this->theme = (EditorTheme)theme_index;
            changed = true;
        }

        changed |= ImGui::SliderFloat("UI Scale", &this->ui_scale, 0.5f, 2.0f, "%.2f");
        changed |= ImGui::SliderFloat("Window Rounding", &this->window_rounding, 0.0f, 12.0f, "%.0f");
        changed |= ImGui::SliderFloat("Frame Rounding", &this->frame_rounding, 0.0f, 12.0f, "%.0f");

        ImGui::Spacing();
        if (ImGui::Button("Reset Theme")) {
            this->default_theme_settings();
            changed = true;
        }

        if (changed) {
            this->apply_theme();
        }
    }

    void EditorSettingsPopup::draw_text_editor_settings()
    {
        ImGui::SeparatorText("Font");

        ImGui::SliderInt("Font Size", &this->font_size, 8, 32);

        ImGui::SeparatorText("Indentation");

        ImGui::SliderInt("Tab Size", &this->tab_size, 1, 8);
        ImGui::Checkbox("Insert Spaces", &this->insert_spaces);

        ImGui::SeparatorText("Display");

        ImGui::Checkbox("Show Line Numbers", &this->show_line_numbers);
        ImGui::Checkbox("Word Wrap", &this->word_wrap);
        ImGui::Checkbox("Highlight Current Line", &this->highlight_current_line);

        ImGui::Spacing();
        if (ImGui::Button("Reset Text Editor")) {
            this->default_text_editor_settings();
        }
    }

    void EditorSettingsPopup::draw_action_buttons()
    {
        if (ImGui::Button("Close")) {
            this->close();
        }
    }
}
