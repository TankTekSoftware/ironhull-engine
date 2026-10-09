#include <Rivet/theme/DefaultTheme.hpp>

namespace Rivet
{
    static ImVec4 with_alpha(const ImVec4& color, float alpha)
    {
        return ImVec4(color.x, color.y, color.z, alpha);
    }

    void DefaultTheme::apply()
    {
        // Start from the dark theme so any colors not set below still have sane values.
        ImGui::StyleColorsDark();

        // Neutral slate grays, darkest to lightest, with a blue accent.
        const ImVec4 crust        = ImVec4(0.067f, 0.071f, 0.078f, 1.00f); // #111214
        const ImVec4 mantle       = ImVec4(0.090f, 0.094f, 0.106f, 1.00f); // #17181b
        const ImVec4 base         = ImVec4(0.114f, 0.122f, 0.137f, 1.00f); // #1d1f23
        const ImVec4 frame        = ImVec4(0.153f, 0.165f, 0.188f, 1.00f); // #272a30
        const ImVec4 surface      = ImVec4(0.188f, 0.200f, 0.227f, 1.00f); // #30333a
        const ImVec4 overlay      = ImVec4(0.227f, 0.243f, 0.275f, 1.00f); // #3a3e46
        const ImVec4 highlight    = ImVec4(0.275f, 0.290f, 0.325f, 1.00f); // #464a53
        const ImVec4 border       = ImVec4(0.196f, 0.208f, 0.235f, 1.00f); // #32353c
        const ImVec4 text         = ImVec4(0.890f, 0.898f, 0.910f, 1.00f); // #e3e5e8
        const ImVec4 subtext      = ImVec4(0.545f, 0.565f, 0.600f, 1.00f); // #8b9099
        const ImVec4 accent       = ImVec4(0.310f, 0.561f, 0.969f, 1.00f); // #4f8ff7
        const ImVec4 accent_light = ImVec4(0.420f, 0.631f, 0.976f, 1.00f); // #6ba1f9

        ImVec4* colors = ImGui::GetStyle().Colors;

        // --- TEXT --- //
        colors[ImGuiCol_Text]                      = text;
        colors[ImGuiCol_TextDisabled]              = subtext;
        colors[ImGuiCol_TextLink]                  = accent_light;
        colors[ImGuiCol_TextSelectedBg]            = with_alpha(accent, 0.35f);
        colors[ImGuiCol_InputTextCursor]           = text;

        // --- WINDOWS --- //
        colors[ImGuiCol_WindowBg]                  = base;
        colors[ImGuiCol_ChildBg]                   = with_alpha(base, 0.00f);
        colors[ImGuiCol_PopupBg]                   = with_alpha(frame, 0.98f);
        colors[ImGuiCol_Border]                    = border;
        colors[ImGuiCol_BorderShadow]              = with_alpha(crust, 0.00f);
        colors[ImGuiCol_TitleBg]                   = crust;
        colors[ImGuiCol_TitleBgActive]             = mantle;
        colors[ImGuiCol_TitleBgCollapsed]          = crust;
        colors[ImGuiCol_MenuBarBg]                 = mantle;
        colors[ImGuiCol_ModalWindowDimBg]          = with_alpha(crust, 0.65f);

        // --- FRAMES (inputs, sliders, checkboxes) --- //
        colors[ImGuiCol_FrameBg]                   = frame;
        colors[ImGuiCol_FrameBgHovered]            = surface;
        colors[ImGuiCol_FrameBgActive]             = overlay;
        colors[ImGuiCol_CheckMark]                 = accent_light;
        colors[ImGuiCol_CheckboxSelectedBg]        = with_alpha(accent, 0.20f);
        colors[ImGuiCol_SliderGrab]                = accent;
        colors[ImGuiCol_SliderGrabActive]          = accent_light;

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
        colors[ImGuiCol_TabHovered]                = overlay;
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
        colors[ImGuiCol_PlotLinesHovered]          = accent_light;
        colors[ImGuiCol_PlotHistogram]             = accent;
        colors[ImGuiCol_PlotHistogramHovered]      = accent_light;

        // --- TABLES --- //
        colors[ImGuiCol_TableHeaderBg]             = frame;
        colors[ImGuiCol_TableBorderStrong]         = border;
        colors[ImGuiCol_TableBorderLight]          = with_alpha(border, 0.60f);
        colors[ImGuiCol_TableRowBg]                = with_alpha(base, 0.00f);
        colors[ImGuiCol_TableRowBgAlt]             = with_alpha(text, 0.03f);
        colors[ImGuiCol_TreeLines]                 = border;

        // --- DRAG & DROP / NAVIGATION --- //
        colors[ImGuiCol_DragDropTarget]            = accent;
        colors[ImGuiCol_DragDropTargetBg]          = with_alpha(accent, 0.12f);
        colors[ImGuiCol_UnsavedMarker]             = accent_light;
        colors[ImGuiCol_NavCursor]                 = accent;
        colors[ImGuiCol_NavWindowingHighlight]     = with_alpha(text, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]         = with_alpha(crust, 0.60f);
    }
}
