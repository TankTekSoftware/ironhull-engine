#include "IronHull/asset/AssetRegistry.hpp"
#include "IronHull/io/FileSystem.hpp"
#include "IronHull/render/RenderPass.hpp"
#include "IronHull/scene/SceneManager.hpp"
#include <IronHull/core/Application.hpp>

#include <raylib.h>
#include <rlImGui.h>

#if defined(PLATFORM_WEB)
#include <emscripten.h>
#endif

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
#else
        return true;
#endif        
    }

    Window Application::get_window()
    {
        Application* self = Application::instance;
        return self->instance->window;
    }

    Viewport Application::get_viewport()
    {
        Application* self = Application::instance;
        return self->instance->viewport;
    }

    Physics2DSettings Application::get_physics2d()
    {
        Application* self = Application::instance;
        return self->instance->physics_2d;
    }

    void Application::run()
    {
        Application::instance = this;
        this->is_running = true;
        this->window = { 0 };
        this->window.width = 1280;
        this->window.height = 720;
        this->window.title = "Untitled";
        this->window.resizable = false;
        this->window.fullscreen = false;
        this->window.target_fps = 0;

        this->viewport = { 0 };
        this->viewport.width = 1280;
        this->viewport.height = 720;

        this->physics_2d = { 0 };
        this->physics_2d.gravity = { 0.0f, 9.8f };
        this->physics_2d.pixels_per_unit = 32.0f;

        this->project_name = "IronHullGame";

        this->compose();
        this->target = LoadRenderTexture(this->viewport.width, this->viewport.height);

        this->ready();

#if defined(PLATFORM_WEB)
        // The browser owns the event loop on web: a blocking while-loop like the desktop one
        // below would never yield back to it, freezing the tab and starving rendering/input.
        // emscripten_set_main_loop_arg() instead hands it a per-frame callback and unwinds
        // the stack out of run() (and main()) without running anything after this call -
        // Application::web_frame() takes over as the loop and calls dispose() itself once
        // the app stops running.
        emscripten_set_main_loop_arg(&Application::web_frame, this, 0, true);
#else
        while (!WindowShouldClose() && this->is_running) {
            this->frame();
        }

        this->dispose();
#endif
    }

    void Application::frame()
    {
        float delta = GetFrameTime();
        this->update(delta);

        if (IsWindowResized()) {
            this->window.width = GetScreenWidth();
            this->window.height = GetScreenHeight();
        }

        Rectangle sourceRect = {
            0.0f, 0.0f,
            static_cast<float>(this->target.texture.width),
            static_cast<float>(-this->target.texture.height)
        };

        Rectangle destRect = {
            0.0f, 0.0f,
            static_cast<float>(Application::get_window().width),
            static_cast<float>(Application::get_window().height)
        };

        BeginTextureMode(this->target);
        {
            ClearBackground(BLACK);
            this->draw(RenderPass::VIEWPORT);
        }
        EndTextureMode();

        BeginDrawing();
        {
            ClearBackground(BLACK);
            DrawTexturePro(this->target.texture, sourceRect, destRect, { 0.0f, 0.0f }, 0.0f, WHITE);

            this->draw(RenderPass::SCREEN);

            rlImGuiBegin();
            {
                this->draw(RenderPass::DEBUG);
            }
            rlImGuiEnd();
        }
        EndDrawing();
    }

#if defined(PLATFORM_WEB)
    void Application::web_frame(void* arg)
    {
        Application* self = static_cast<Application*>(arg);

        // Deliberately not calling WindowShouldClose() here: raylib's web backend implements
        // it with an internal emscripten_sleep() (to give control back to the browser in the
        // synchronous while-loop porting pattern), which requires the binary to be built with
        // -sASYNCIFY - without it, the very first call aborts the module. It always returns
        // false anyway when driven via emscripten_set_main_loop_arg() like this, so is_running
        // is the only real quit signal on web.
        if (!self->is_running) {
            emscripten_cancel_main_loop();
            self->dispose();
            return;
        }

        self->frame();
    }
#endif

    void Application::compose()
    {
        this->on_compose();

        FileSystem::init(this->project_name);

        unsigned int flags = 0;
        if (this->window.resizable) {
            flags |= FLAG_WINDOW_RESIZABLE;
        }

        flags |= FLAG_WINDOW_HIGHDPI;
        flags |= FLAG_MSAA_4X_HINT;

        SetConfigFlags(flags);

        InitWindow(this->window.width, this->window.height, this->window.title.c_str());
        InitAudioDevice();
        SetExitKey(KEY_NULL);

        if (this->window.target_fps == 0) {
            this->window.target_fps = GetMonitorRefreshRate(GetCurrentMonitor());  
        }
        SetTargetFPS(this->window.target_fps);

        if (this->window.fullscreen) {
            ToggleFullscreen();
            this->window.width = GetScreenWidth();
            this->window.height = GetScreenHeight();
        }

        rlImGuiSetup(true);
    }

    void Application::ready()
    {
        SceneManager::ready();
        this->on_ready();
    }

    void Application::update(float delta)
    {
        SceneManager::update(delta);
        this->on_update(delta);
    }

    void Application::draw(RenderPass pass)
    {
        SceneManager::draw(pass);
        this->on_draw(pass);
    }

    void Application::dispose()
    {
        SceneManager::dispose();
        this->on_dispose();

        AssetRegistry::dispose();
        FileSystem::shutdown();
        rlImGuiShutdown();
        CloseAudioDevice();
        CloseWindow();
    }

}
