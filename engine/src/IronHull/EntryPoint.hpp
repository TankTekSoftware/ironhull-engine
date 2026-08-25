#pragma once

#ifdef PLATFORM_DESKTOP

#include <IronHull/core/Application.hpp>

extern IronHull::Application* IronHull::create_application();

int main(int argc, char* argv[])
{
    IronHull::Application* app = IronHull::create_application();
    app->run();
    delete app;
    return 0;
}

#endif
