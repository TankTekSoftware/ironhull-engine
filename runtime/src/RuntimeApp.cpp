#include <IronHull/IronHull.hpp>
#include <IronHull/EntryPoint.hpp>

#include <MapScene.hpp>

class RuntimeApp : public IronHull::Application
{
    protected:
        void on_compose() override
        {
            this->window.width = 1280;
            this->window.height = 720;
            this->window.title = "IronHull Runtime";
            this->window.resizable = true;

            this->viewport.width = 1280;
            this->viewport.height = 720;

            this->project_name = "IronHullSandbox";

            IronHull::SceneManager::register_scene<MapScene>("map");
        }

        void on_ready() override
        {
            IronHull::SceneManager::change_scene("map");
        }
};

IronHull::Application* IronHull::create_application()
{
    return new RuntimeApp();
}
