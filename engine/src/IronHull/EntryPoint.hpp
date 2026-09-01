#pragma once

#if defined(PLATFORM_DESKTOP) || defined(PLATFORM_WEB)

#include <IronHull/core/Application.hpp>

extern IronHull::Application* IronHull::create_application();

int main(int argc, char* argv[])
{
    IronHull::Application* app = IronHull::create_application();
    app->run();
    // On PLATFORM_WEB, Application::run() hands the frame loop to the browser via
    // emscripten_set_main_loop_arg() and never returns (it unwinds the stack out of main()
    // instead), so this cleanup only ever runs on desktop - the web build's equivalent
    // teardown happens from Application::web_frame() once the app stops running.
    delete app;
    return 0;
}

#endif
