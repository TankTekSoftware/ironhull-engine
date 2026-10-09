#include <IronHull/IronHull.hpp>
#include <IronHull/EntryPoint.hpp>

#include <Rivet/ui/MenuBar.hpp>

#include <Rivet/panel/EntitiesPanel.hpp>
#include <Rivet/panel/InspectorPanel.hpp>
#include <Rivet/panel/AssetPanel.hpp>
#include <Rivet/panel/ViewportPanel.hpp>

class RivetApp : public IronHull::Application
{
    private:
        Rivet::MenuBar* menu_bar;    
        Rivet::EntitiesPanel* entities_panel;
        Rivet::InspectorPanel* inspector_panel;
        Rivet::AssetPanel* asset_panel;
        Rivet::ViewportPanel* viewport_panel;

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
            
            this->inspector_panel = new Rivet::InspectorPanel();
            this->entities_panel = new Rivet::EntitiesPanel();
            this->asset_panel = new Rivet::AssetPanel();
            this->viewport_panel = new Rivet::ViewportPanel();
        }

        void on_ready() override
        {
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

            // --- REGISTER MENU BAR CALLBACKS --- //

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
                this->inspector_panel->draw();
                this->asset_panel->draw();

                this->viewport_panel->draw();

                // --- DRAW POPUPS --- //
                
            }
        }

        void on_dispose() override
        {
            delete this->menu_bar;

            delete this->entities_panel;
            delete this->inspector_panel;
            delete this->asset_panel;

            delete this->viewport_panel;
        }
};

IronHull::Application* IronHull::create_application()
{
    return new RivetApp();
}
