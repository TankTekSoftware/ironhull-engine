#include <Rivet/theme/LightTheme.hpp>

namespace Rivet
{
    static ImVec4 with_alpha(const ImVec4& color, float alpha)
    {
        return ImVec4(color.x, color.y, color.z, alpha);
    }

    void LightTheme::apply()
    {
        // Start from the light theme so any colors not set below still have sane values.
        ImGui::StyleColorsLight();

        // Cool light grays with a darker blue accent, so it stays readable on white.
        const ImVec4 crust        = ImVec4(0.851f, 0.863f, 0.882f, 1.00f); // #d9dce1
        const ImVec4 mantle       = ImVec4(0.902f, 0.910f, 0.925f, 1.00f); // #e6e8ec
        const ImVec4 frame        = ImVec4(0.894f, 0.902f, 0.918f, 1.00f); // #e4e6ea
        const ImVec4 surface      = ImVec4(0.855f, 0.867f, 0.886f, 1.00f); // #dadde2
        const ImVec4 overlay      = ImVec4(0.800f, 0.816f, 0.839f, 1.00f); // #ccd0d6
        const ImVec4 highlight    = ImVec4(0.722f, 0.741f, 0.773f, 1.00f); // #b8bdc5
        const ImVec4 border       = ImVec4(0.816f, 0.827f, 0.851f, 1.00f); // #d0d3d9
        const ImVec4 base         = ImVec4(0.965f, 0.969f, 0.976f, 1.00f); // #f6f7f9
        const ImVec4 paper        = ImVec4(1.000f, 1.000f, 1.000f, 1.00f); // #ffffff
        const ImVec4 text         = ImVec4(0.122f, 0.137f, 0.161f, 1.00f); // #1f2329
        const ImVec4 subtext      = ImVec4(0.420f, 0.443f, 0.482f, 1.00f); // #6b717b
        const ImVec4 accent       = ImVec4(0.184f, 0.435f, 0.871f, 1.00f); // #2f6fde
        const ImVec4 accent_dark  = ImVec4(0.122f, 0.357f, 0.769f, 1.00f); // #1f5bc4

        ImVec4* colors = ImGui::GetStyle().Colors;

        // --- TEXT --- //
        colors[ImGuiCol_Text]                      = text;
        colors[ImGuiCol_TextDisabled]              = subtext;
        colors[ImGuiCol_TextLink]                  = accent_dark;
        colors[ImGuiCol_TextSelectedBg]            = with_alpha(accent, 0.35f);
        colors[ImGuiCol_InputTextCursor]           = text;

        // --- WINDOWS --- //
        colors[ImGuiCol_WindowBg]                  = base;
        colors[ImGuiCol_ChildBg]                   = with_alpha(base, 0.00f);
        colors[ImGuiCol_PopupBg]                   = with_alpha(paper, 0.98f);
        colors[ImGuiCol_Border]                    = border;
        colors[ImGuiCol_BorderShadow]              = with_alpha(crust, 0.00f);
        colors[ImGuiCol_TitleBg]                   = crust;
        colors[ImGuiCol_TitleBgActive]             = mantle;
        colors[ImGuiCol_TitleBgCollapsed]          = crust;
        colors[ImGuiCol_MenuBarBg]                 = mantle;
        colors[ImGuiCol_ModalWindowDimBg]          = with_alpha(text, 0.35f);

        // --- FRAMES (inputs, sliders, checkboxes) --- //
        colors[ImGuiCol_FrameBg]                   = frame;
        colors[ImGuiCol_FrameBgHovered]            = surface;
        colors[ImGuiCol_FrameBgActive]             = overlay;
        colors[ImGuiCol_CheckMark]                 = accent_dark;
        colors[ImGuiCol_CheckboxSelectedBg]        = with_alpha(accent, 0.20f);
        colors[ImGuiCol_SliderGrab]                = accent;
        colors[ImGuiCol_SliderGrabActive]          = accent_dark;

        // --- BUTTONS --- //
        colors[ImGuiCol_Button]                    = surface;
        colors[ImGuiCol_ButtonHovered]             = overlay;
        colors[ImGuiCol_ButtonActive]              = with_alpha(accent, 0.65f);

        // --- HEADERS (selectables, tree nodes, menu items) --- //
        colors[ImGuiCol_Header]                    = with_alpha(accent, 0.28f);
        colors[ImGuiCol_HeaderHovered]             = with_alpha(accent, 0.40f);
        colors[ImGuiCol_HeaderActive]              = with_alpha(accent, 0.55f);

        // --- SEPARATORS & RESIZE GRIPS --- //
        colors[ImGuiCol_Separator]                 = border;
        colors[ImGuiCol_SeparatorHovered]          = with_alpha(accent, 0.70f);
        colors[ImGuiCol_SeparatorActive]           = accent;
        colors[ImGuiCol_ResizeGrip]                = with_alpha(overlay, 0.40f);
        colors[ImGuiCol_ResizeGripHovered]         = with_alpha(accent, 0.60f);
        colors[ImGuiCol_ResizeGripActive]          = accent;

        // --- SCROLLBARS --- //
        colors[ImGuiCol_ScrollbarBg]               = with_alpha(base, 0.00f);
        colors[ImGuiCol_ScrollbarGrab]             = overlay;
        colors[ImGuiCol_ScrollbarGrabHovered]      = highlight;
        colors[ImGuiCol_ScrollbarGrabActive]       = subtext;

        // --- TABS --- //
        colors[ImGuiCol_Tab]                       = mantle;
        colors[ImGuiCol_TabHovered]                = surface;
        colors[ImGuiCol_TabSelected]               = base;
        colors[ImGuiCol_TabSelectedOverline]       = accent;
        colors[ImGuiCol_TabDimmed]                 = mantle;
        colors[ImGuiCol_TabDimmedSelected]         = base;
        colors[ImGuiCol_TabDimmedSelectedOverline] = with_alpha(subtext, 0.40f);

        // --- DOCKING --- //
        colors[ImGuiCol_DockingPreview]            = with_alpha(accent, 0.45f);
        colors[ImGuiCol_DockingEmptyBg]            = crust;

        // --- PLOTS --- //
        colors[ImGuiCol_PlotLines]                 = subtext;
        colors[ImGuiCol_PlotLinesHovered]          = accent_dark;
        colors[ImGuiCol_PlotHistogram]             = accent;
        colors[ImGuiCol_PlotHistogramHovered]      = accent_dark;

        // --- TABLES --- //
        colors[ImGuiCol_TableHeaderBg]             = frame;
        colors[ImGuiCol_TableBorderStrong]         = border;
        colors[ImGuiCol_TableBorderLight]          = with_alpha(border, 0.60f);
        colors[ImGuiCol_TableRowBg]                = with_alpha(base, 0.00f);
        colors[ImGuiCol_TableRowBgAlt]             = with_alpha(text, 0.04f);
        colors[ImGuiCol_TreeLines]                 = border;

        // --- DRAG & DROP / NAVIGATION --- //
        colors[ImGuiCol_DragDropTarget]            = accent;
        colors[ImGuiCol_DragDropTargetBg]          = with_alpha(accent, 0.12f);
        colors[ImGuiCol_UnsavedMarker]             = accent_dark;
        colors[ImGuiCol_NavCursor]                 = accent;
        colors[ImGuiCol_NavWindowingHighlight]     = with_alpha(text, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]         = with_alpha(text, 0.20f);
    }
}
