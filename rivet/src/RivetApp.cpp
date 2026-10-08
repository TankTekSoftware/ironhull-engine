#include <IronHull/IronHull.hpp>
#include <IronHull/EntryPoint.hpp>

#include <Rivet/ui/MenuBar.hpp>

#include <Rivet/panel/EntitiesPanel.hpp>

#include <Rivet/popup/NewProjectPopup.hpp>
#include <Rivet/popup/ProjectSettingsPopup.hpp>

class RivetApp : public IronHull::Application
{
    private:
        Rivet::MenuBar* menu_bar;    
        Rivet::EntitiesPanel* entities_panel;

        Rivet::NewProjectPopup* new_project_popup;
        Rivet::ProjectSettingsPopup* project_settings_popup;
    protected:
        void on_compose() override
        {
            this->window.width = 1280;
            this->window.height = 720;
            this->window.title = "Rivet Editor";
            this->window.target_fps = 0;
            this->window.resizable = true;

            this->viewport.width = 1280;
            this->viewport.height = 720;

            this->project_name = "RivetEditor";

            this->menu_bar = new Rivet::MenuBar();
            
            this->entities_panel = new Rivet::EntitiesPanel();

            this->new_project_popup = new Rivet::NewProjectPopup();
            this->project_settings_popup = new Rivet::ProjectSettingsPopup();
        }

        void on_ready() override
        {
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

            // --- REGISTER MENU BAR CALLBACKS --- //
            this->menu_bar->set_open_new_project([this]() { this->new_project_popup->open(); });
            this->menu_bar->set_open_project_settings([this]() { this->project_settings_popup->open(); });
        }

        void on_update(float delta) override
        {

        }

        void on_draw(IronHull::RenderPass pass) override
        {
            if (pass == IronHull::RenderPass::IMGUI) {
                // This is what shows the docking spaces.
                ImGui::DockSpaceOverViewport(0,  NULL, ImGuiDockNodeFlags_PassthruCentralNode);
                
                this->menu_bar->draw();
                
                // --- DRAW PANELS --- //
                this->entities_panel->draw();

                // --- DRAW POPUPS --- //
                this->new_project_popup->draw();
                this->project_settings_popup->draw();
            }
        }

        void on_dispose() override
        {
            delete this->menu_bar;
            delete this->entities_panel;

            delete this->new_project_popup;
            delete this->project_settings_popup;
        }
};

IronHull::Application* IronHull::create_application()
{
    return new RivetApp();
}
