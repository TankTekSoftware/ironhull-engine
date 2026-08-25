#include "IronHull/render/RenderPass.hpp"
#include "IronHull/scene/SceneManager.hpp"
#include <IronHull/core/Application.hpp>

#include <raylib.h>
#include <rlImGui.h>

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

        this->project_name = "IronHullGame";

        this->compose(); 
        RenderTexture2D target = LoadRenderTexture(this->viewport.width, this->viewport.height);

        this->ready();

        while (!WindowShouldClose() && this->is_running) {
            float delta = GetFrameTime();
            this->update(delta);

            if (IsWindowResized()) {
                this->window.width = GetScreenWidth();
                this->window.height = GetScreenHeight();
            }

            Rectangle sourceRect = { 0.0f, 0.0f, static_cast<float>(target.texture.width), static_cast<float>(-target.texture.height) };
            Rectangle destRect = { 0.0f, 0.0f, static_cast<float>(Application::get_window_width()), static_cast<float>(Application::get_window_height()) };

            BeginTextureMode(target);
            {
                ClearBackground(BLACK);
                this->draw(RenderPass::VIEWPORT);
            }
            EndTextureMode();

            BeginDrawing();
            {
                ClearBackground(BLACK);
                DrawTexturePro(target.texture, sourceRect, destRect, { 0.0f, 0.0f }, 0.0f, WHITE);

                this->draw(RenderPass::SCREEN);

                if (Application::is_debug()) {
                    rlImGuiBegin();
                    {
                        this->draw(RenderPass::DEBUG);
                    }
                    rlImGuiEnd();
                }
            }
            EndDrawing();
        }

        this->dispose();
    }

    void Application::compose()
    {
        this->on_compose();

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
        rlImGuiShutdown();
        this->on_dispose();
        CloseAudioDevice();
        CloseWindow();
    }

}
