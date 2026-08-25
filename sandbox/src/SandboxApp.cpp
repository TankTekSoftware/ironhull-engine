#include "IronHull/core/Application.hpp"
#include "IronHull/render/RenderPass.hpp"
#include <IronHull/EntryPoint.hpp>

class SandboxApp : public IronHull::Application
{
    protected:
        void on_compose() override
        {
            
        }

        void on_ready() override
        {

        }

        void on_update(float delta) override
        {

        }

        void on_draw(IronHull::RenderPass pass) override
        {

        }

        void on_dispose() override
        {

        }
};

IronHull::Application* IronHull::create_application()
{
    return new SandboxApp();
}
