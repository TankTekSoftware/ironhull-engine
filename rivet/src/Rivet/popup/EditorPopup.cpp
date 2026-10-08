#include <Rivet/popup/EditorPopup.hpp>

namespace Rivet
{
    EditorPopup::EditorPopup(const std::string& title, ImGuiWindowFlags flags)
    {
        this->title = title;
        this->flags = flags;
        this->visible = false;
        this->open_requested = false;

    }

    EditorPopup::~EditorPopup()
    {

    }

    void EditorPopup::on_open()
    {

    }

    void EditorPopup::on_close()
    {

    }

    void EditorPopup::draw()
    {
        // ImGui::OpenPopup() has to be called from the same ID stack as BeginPopupModal(),
        // so open() only queues the request and it's actually opened here.
        if (this->open_requested) {
            ImGui::OpenPopup(this->title.c_str());
            this->open_requested = false;
        }

        bool was_visible = this->visible;

        if (ImGui::BeginPopupModal(this->title.c_str(), &this->visible, this->flags)) {
            this->on_draw();

            if (!this->visible) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        else {
            // Closed by ImGui itself (title bar X, escape, etc).
            this->visible = false;
        }

        if (was_visible && !this->visible) {
            this->on_close();
        }
    }

    void EditorPopup::open()
    {
        if (this->visible) {
            return;
        }

        this->visible = true;
        this->open_requested = true;
        this->on_open();
    }

    void EditorPopup::close()
    {
        this->visible = false;
    }

    bool EditorPopup::is_open() const
    {
        return this->visible;
    }
}
