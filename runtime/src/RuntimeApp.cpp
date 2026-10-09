#include <IronHull/IronHull.hpp>
#include <IronHull/EntryPoint.hpp>

class RuntimeApp : public IronHull::Application
{
    protected:
        void on_compose() override
        {}

        void on_ready() override
        {}

        void on_update(float delta) override
        {}

        void on_draw(IronHull::RenderPass pass) override
        {}

        void on_dispose() override
        {}
};

IronHull::Application* IronHull::create_application()
{
    return new RuntimeApp();
}
