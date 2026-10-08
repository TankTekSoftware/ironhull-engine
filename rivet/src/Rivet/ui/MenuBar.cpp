#include <Rivet/ui/MenuBar.hpp>
#include <IronHull/IronHull.hpp>

#include <iostream>

#include <imgui.h>

namespace Rivet
{
    MenuBar::MenuBar()
    {

    }

    MenuBar::~MenuBar()
    {

    }

    void MenuBar::draw()
    {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Project")) {
                    this->open_new_project();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Project")) {
                if (ImGui::MenuItem("Project Settings")) {
                    if (!this->open_project_settings) {
                        std::cerr << "ERROR: MENU_BAR: open_project_settings() is not defined." << std::endl;
                        return;
                    }
                    this->open_project_settings();
                }
                if (ImGui::MenuItem("Close Project")) {
                    IronHull::Application::quit();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    void MenuBar::set_open_new_project(const std::function<void()> callback)
    {
        this->open_new_project = callback;
    }

    void MenuBar::set_open_project_settings(const std::function<void()> callback)
    {
        this->open_project_settings = callback;
    }
}

