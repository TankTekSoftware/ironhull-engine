#pragma once

#include <IronHull/IronHull.hpp>

// Walks around a compiled map.
//
// This is the worked example of the map and entity systems in use: it loads the entity
// scripts, loads a compiled map, puts the player where the map's info_player_start says, and
// moves them around by sweeping their box through the BSP tree. The entities tick themselves
// and carry their own I/O.
class MapScene : public IronHull::Scene
{
    private:
        IronHull::Map map;

        Camera3D camera = { 0 };

        // The player's feet, in engine space. Their box is measured up from here.
        Vector3 position = { 0.0f, 0.0f, 0.0f };
        Vector3 velocity = { 0.0f, 0.0f, 0.0f };

        // Pitch, yaw and roll in degrees, in the same convention an entity's `angles` uses,
        // so the same code turns both into a direction.
        Vector3 look = { 0.0f, 0.0f, 0.0f };

        bool on_ground = false;
        bool mouse_captured = false;

        // Fly through walls, to look at how a map was built.
        bool noclip = false;
        bool wireframe = false;

        bool map_ready = false;
        std::string load_error;

    protected:
        void on_ready() override;
        void on_update(float delta) override;
        void on_draw(IronHull::RenderPass pass) override;
        void on_dispose() override;

    private:
        void register_input();

        void update_look();
        void update_movement(float delta);

        // Advances the player by their velocity, sliding along whatever they run into rather
        // than stopping dead against it.
        void move_with_collision(float delta);

        void update_camera();
        void draw_hud();
};
