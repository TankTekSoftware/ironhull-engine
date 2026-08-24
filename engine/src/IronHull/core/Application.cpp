#include <IronHull/core/Application.hpp>

namespace IronHull
{
    Application* Application::instance = nullptr;

    void Application::quit()
    {
        Application::instance->is_running = false;
    }

    bool Application::is_debug()
    {
        #ifdef NDEBUG
            return false;
        #endif        
        return true;
    }

    int Application::get_window_width()
    {
        Application* self = Application::instance;
        return self->window.width; 
    }

    int Application::get_window_height()
    {
        Application* self = Application::instance;
        return self->window.height;
    }

    int Application::get_viewport_width()
    {
        Application* self = Application::instance;
        return self->viewport.width;
    }
    
    int Application::get_viewport_height()
    {
        Application* self = Application::instance;
        return self->viewport.height;
    }

}
