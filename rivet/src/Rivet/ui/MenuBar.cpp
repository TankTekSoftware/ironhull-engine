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
            this->draw_map_menu();
            this->draw_project_menu();

            ImGui::EndMainMenuBar();
        }
    }

    void MenuBar::draw_map_menu()
    {
        if (ImGui::BeginMenu("Map")) {
            if (ImGui::MenuItem("New Map")) {
                // TODO: Create a new map document.
            }
            if (ImGui::MenuItem("Open Map")) {
                // TODO: Open an exisiting map document.
            }
            if (ImGui::BeginMenu("Open Recent")) {
                // TODO: List all recently opened maps. 

                ImGui::Separator();
                
                if (ImGui::MenuItem("Clear Recent Maps")) {
                    
                }

                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Save Map")) {
                // TODO: Save the currently opened map.
            }

            if (ImGui::MenuItem("Save Map As")) {
                // TODO: Save the currently opened map to a specific location.
            }                

            if (ImGui::MenuItem("Save All Map")) {
                // TODO: Loop through all unsaved maps and save them.
            }

            ImGui::Separator();
            
            if (ImGui::MenuItem("Undo")) {
                
            }

            if (ImGui::MenuItem("Redo")) {

            }

            ImGui::Separator();

            if (ImGui::MenuItem("Close Map")) {

            }

            if (ImGui::MenuItem("Close All Scenes")) {

            }

            ImGui::Separator();

            if (ImGui::MenuItem("Quit")) {
                IronHull::Application::quit();
            }

            ImGui::EndMenu();
        }
    }

    void MenuBar::draw_project_menu()
    {
        if (ImGui::BeginMenu("Project")) {
            if (ImGui::MenuItem("Project Settings")) { 
                // TODO: Open the project settings popup
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Export")) {
                // TODO: Open the export project popup
            }      
            
            ImGui::EndMenu();
        }
    }

    void MenuBar::draw_editor_menu()
    {
        if (ImGui::BeginMenu("Editor")) {
            if (ImGui::MenuItem("Editor Settings")) {
                // TODO: Open the editor settings.
            }
            ImGui::EndMenu();
        }
    }
}

