#include <Rivet/widget/ImGuiFileInput.hpp>

#include <cstring>

#include <imgui.h>
#include <nfd.hpp>

namespace Rivet
{
    ImGuiFileInput::ImGuiFileInput(const std::string& label, FileInputMode mode)
    {
        this->label = label;
        this->mode = mode;
        
        this->clear();
    }

    bool ImGuiFileInput::draw()
    {
        bool changed = ImGui::InputText(this->label.c_str(), this->path, IM_ARRAYSIZE(this->path));

        ImGui::SameLine();
        ImGui::PushID(this->label.c_str());
        if (ImGui::Button("...")) {
            changed |= this->browse();
        }
        ImGui::PopID();

        return changed;
    }

    void ImGuiFileInput::clear()
    {
        this->path[0] = '\0';
    }

    const char* ImGuiFileInput::get_path() const
    {
        return this->path;
    }

    void ImGuiFileInput::set_path(const char* path)
    {
        std::strncpy(this->path, path, IM_ARRAYSIZE(this->path) - 1);
        this->path[IM_ARRAYSIZE(this->path) - 1] = '\0';
    }

    bool ImGuiFileInput::is_empty() const
    {
        return this->path[0] == '\0';
    }

    bool ImGuiFileInput::browse()
    {
        NFD::Guard nfd_guard;
        NFD::UniquePath out_path;
        const nfdu8char_t* default_path = !this->is_empty() ? this->path : nullptr;

        nfdresult_t result = NFD_CANCEL;
        switch (this->mode) {
            case FileInputMode::MODE_FOLDER:
                result = NFD::PickFolder(out_path, default_path);
                break;
            case FileInputMode::MODE_OPEN_FILE:
                result = NFD::OpenDialog(out_path, nullptr, 0, default_path);
                break;
            case FileInputMode::MODE_SAVE_FILE:
                result = NFD::SaveDialog(out_path, nullptr, 0, default_path);
                break;
        }

        if (result != NFD_OKAY) {
            return false;
        }

        this->set_path(out_path.get());
        return true;
    }
}
