#include <Rivet/theme/SrcTheme.hpp>

namespace Rivet
{
    static ImVec4 with_alpha(const ImVec4& color, float alpha)
    {
        return ImVec4(color.x, color.y, color.z, alpha);
    }

    void SrcTheme::apply()
    {
        // Start from the dark theme so any colors not set below still have sane values.
        ImGui::StyleColorsDark();

        const ImVec4 accent  = ImVec4(0.984f, 0.494f, 0.078f, 1.00f); // #fb7e14
        const ImVec4 black   = ImVec4(0.000f, 0.000f, 0.000f, 1.00f); // #000000
        const ImVec4 base    = ImVec4(0.082f, 0.082f, 0.082f, 1.00f); // #151515
        const ImVec4 surface = ImVec4(0.161f, 0.161f, 0.161f, 1.00f); // #292929
        const ImVec4 white   = ImVec4(1.000f, 1.000f, 1.000f, 1.00f); // #ffffff

        ImVec4* colors = ImGui::GetStyle().Colors;

        // --- TEXT --- //
        colors[ImGuiCol_Text]                      = white;
        colors[ImGuiCol_TextDisabled]              = with_alpha(white, 0.45f);
        colors[ImGuiCol_TextLink]                  = accent;
        colors[ImGuiCol_TextSelectedBg]            = with_alpha(accent, 0.35f);
        colors[ImGuiCol_InputTextCursor]           = accent;

        // --- WINDOWS --- //
        colors[ImGuiCol_WindowBg]                  = base;
        colors[ImGuiCol_ChildBg]                   = with_alpha(black, 0.00f);
        colors[ImGuiCol_PopupBg]                   = base;
        colors[ImGuiCol_Border]                    = surface;
        colors[ImGuiCol_BorderShadow]              = with_alpha(black, 0.00f);
        colors[ImGuiCol_TitleBg]                   = black;
        colors[ImGuiCol_TitleBgActive]             = black;
        colors[ImGuiCol_TitleBgCollapsed]          = black;
        colors[ImGuiCol_MenuBarBg]                 = black;
        colors[ImGuiCol_ModalWindowDimBg]          = with_alpha(black, 0.60f);

        // --- FRAMES (inputs, sliders, checkboxes) --- //
        colors[ImGuiCol_FrameBg]                   = black;
        colors[ImGuiCol_FrameBgHovered]            = surface;
        colors[ImGuiCol_FrameBgActive]             = surface;
        colors[ImGuiCol_CheckMark]                 = accent;
        colors[ImGuiCol_CheckboxSelectedBg]        = black;
        colors[ImGuiCol_SliderGrab]                = accent;
        colors[ImGuiCol_SliderGrabActive]          = white;

        // --- BUTTONS --- //
        colors[ImGuiCol_Button]                    = surface;
        colors[ImGuiCol_ButtonHovered]             = with_alpha(accent, 0.70f);
        colors[ImGuiCol_ButtonActive]              = accent;

        // --- HEADERS (selectables, tree nodes, menu items) --- //
        colors[ImGuiCol_Header]                    = surface;
        colors[ImGuiCol_HeaderHovered]             = with_alpha(accent, 0.70f);
        colors[ImGuiCol_HeaderActive]              = accent;

        // --- SEPARATORS & RESIZE GRIPS --- //
        colors[ImGuiCol_Separator]                 = surface;
        colors[ImGuiCol_SeparatorHovered]          = with_alpha(accent, 0.70f);
        colors[ImGuiCol_SeparatorActive]           = accent;
        colors[ImGuiCol_ResizeGrip]                = surface;
        colors[ImGuiCol_ResizeGripHovered]         = with_alpha(accent, 0.70f);
        colors[ImGuiCol_ResizeGripActive]          = accent;

        // --- SCROLLBARS --- //
        colors[ImGuiCol_ScrollbarBg]               = black;
        colors[ImGuiCol_ScrollbarGrab]             = surface;
        colors[ImGuiCol_ScrollbarGrabHovered]      = with_alpha(accent, 0.70f);
        colors[ImGuiCol_ScrollbarGrabActive]       = accent;

        // --- TABS --- //
        colors[ImGuiCol_Tab]                       = black;
        colors[ImGuiCol_TabHovered]                = with_alpha(accent, 0.70f);
        colors[ImGuiCol_TabSelected]               = surface;
        colors[ImGuiCol_TabSelectedOverline]       = accent;
        colors[ImGuiCol_TabDimmed]                 = black;
        colors[ImGuiCol_TabDimmedSelected]         = base;
        colors[ImGuiCol_TabDimmedSelectedOverline] = with_alpha(accent, 0.50f);

        // --- DOCKING --- //
        colors[ImGuiCol_DockingPreview]            = with_alpha(accent, 0.50f);
        colors[ImGuiCol_DockingEmptyBg]            = black;

        // --- PLOTS --- //
        colors[ImGuiCol_PlotLines]                 = white;
        colors[ImGuiCol_PlotLinesHovered]          = accent;
        colors[ImGuiCol_PlotHistogram]             = accent;
        colors[ImGuiCol_PlotHistogramHovered]      = white;

        // --- TABLES --- //
        colors[ImGuiCol_TableHeaderBg]             = surface;
        colors[ImGuiCol_TableBorderStrong]         = surface;
        colors[ImGuiCol_TableBorderLight]          = with_alpha(surface, 0.60f);
        colors[ImGuiCol_TableRowBg]                = with_alpha(black, 0.00f);
        colors[ImGuiCol_TableRowBgAlt]             = with_alpha(white, 0.03f);
        colors[ImGuiCol_TreeLines]                 = surface;

        // --- DRAG & DROP / NAVIGATION --- //
        colors[ImGuiCol_DragDropTarget]            = accent;
        colors[ImGuiCol_DragDropTargetBg]          = with_alpha(accent, 0.15f);
        colors[ImGuiCol_UnsavedMarker]             = accent;
        colors[ImGuiCol_NavCursor]                 = accent;
        colors[ImGuiCol_NavWindowingHighlight]     = with_alpha(white, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]         = with_alpha(black, 0.60f);
    }
}
