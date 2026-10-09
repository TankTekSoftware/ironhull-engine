#include <MapScene.hpp>

#include <algorithm>
#include <cmath>

namespace
{
    const char* MAP_URI = "content://maps/testmap.ihbsp";

    // The player's box, in engine space, measured from the feet up. A map-space body 32
    // units across and 72 tall becomes this once Z-up has become Y-up.
    const Vector3 PLAYER_MINS = { -16.0f, 0.0f, -16.0f };
    const Vector3 PLAYER_MAXS = { 16.0f, 72.0f, 16.0f };

    // Map units per second, and units per second squared. These are in the same scale the
    // brushes were authored in, which with the compiler's default scale of 1 is inches.
    constexpr float EYE_HEIGHT = 64.0f;
    constexpr float WALK_SPEED = 260.0f;
    constexpr float NOCLIP_SPEED = 600.0f;
    constexpr float GRAVITY = 800.0f;
    constexpr float JUMP_SPEED = 270.0f;

    constexpr float LOOK_SENSITIVITY = 0.15f;
    constexpr float MAX_PITCH = 89.0f;

    // A surface this close to level is something to stand on rather than slide down.
    constexpr float GROUND_NORMAL = 0.7f;

    // How far down to look for the floor. Enough to find it through the gap a trace leaves
    // when it stops short of a surface, and not enough to find one the player has left.
    constexpr float GROUND_PROBE = 2.0f;
}

void MapScene::on_ready()
{
    this->register_input();

    // Entity definitions have to exist before a map can spawn the classnames it refers to.
    IronHull::EntityRegistry::load_directory();

    try {
        this->map.load(MAP_URI);
        this->map_ready = true;
    } catch (const std::exception& error) {
        this->load_error = error.what();
        TraceLog(LOG_WARNING, "SANDBOX: %s", error.what());
        return;
    }

    Vector3 start_origin = { 0.0f, 0.0f, 0.0f };
    Vector3 start_angles = { 0.0f, 0.0f, 0.0f };

    if (this->map.find_player_start(start_origin, start_angles)) {
        this->position = start_origin;
        this->look = start_angles;
    } else {
        TraceLog(LOG_WARNING, "SANDBOX: '%s' has no info_player_start", MAP_URI);
    }

    this->camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    this->camera.fovy = 90.0f;
    this->camera.projection = CAMERA_PERSPECTIVE;
    this->update_camera();

    this->mouse_captured = true;
    DisableCursor();
}

void MapScene::register_input()
{
    IronHull::Input::register_key_action("move_forward", KEY_W);
    IronHull::Input::register_key_action("move_forward", KEY_UP);
    IronHull::Input::register_key_action("move_back", KEY_S);
    IronHull::Input::register_key_action("move_back", KEY_DOWN);
    IronHull::Input::register_key_action("move_left", KEY_A);
    IronHull::Input::register_key_action("move_right", KEY_D);
    IronHull::Input::register_key_action("jump", KEY_SPACE);
    IronHull::Input::register_key_action("move_up", KEY_SPACE);
    IronHull::Input::register_key_action("move_down", KEY_LEFT_CONTROL);
}

void MapScene::on_update(float delta)
{
    if (!this->map_ready) {
        return;
    }

    // Releasing the mouse makes the window usable again; it is the only way out, since the
    // engine unbinds the escape key from quitting.
    if (IsKeyPressed(KEY_ESCAPE)) {
        this->mouse_captured = !this->mouse_captured;

        if (this->mouse_captured) {
            DisableCursor();
        } else {
            EnableCursor();
        }
    }

    if (IsKeyPressed(KEY_V)) {
        this->noclip = !this->noclip;
        this->velocity = Vector3{ 0.0f, 0.0f, 0.0f };
    }

    if (IsKeyPressed(KEY_F3)) {
        this->wireframe = !this->wireframe;
    }

    this->update_look();
    this->update_movement(delta);
    this->update_camera();

    this->map.update(delta);
}

void MapScene::update_look()
{
    if (!this->mouse_captured) {
        return;
    }

    Vector2 motion = GetMouseDelta();

    // Positive pitch tips downwards in this convention, which is why moving the mouse down
    // adds to it rather than subtracting.
    this->look.x = std::clamp(this->look.x + motion.y * LOOK_SENSITIVITY, -MAX_PITCH, MAX_PITCH);
    this->look.y -= motion.x * LOOK_SENSITIVITY;
}

void MapScene::update_movement(float delta)
{
    // The same angle convention an entity uses, so a camera and an entity facing the same
    // numbers face the same way.
    Vector3 forward = IronHull::MapSpace::angles_to_forward(this->look);
    Vector3 right = IronHull::MapSpace::angles_to_right(this->look);

    Vector3 wish = { 0.0f, 0.0f, 0.0f };

    if (IronHull::Input::is_action_pressed("move_forward")) {
        wish = Vector3Add(wish, forward);
    }

    if (IronHull::Input::is_action_pressed("move_back")) {
        wish = Vector3Subtract(wish, forward);
    }

    if (IronHull::Input::is_action_pressed("move_right")) {
        wish = Vector3Add(wish, right);
    }

    if (IronHull::Input::is_action_pressed("move_left")) {
        wish = Vector3Subtract(wish, right);
    }

    if (this->noclip) {
        if (IronHull::Input::is_action_pressed("move_up")) {
            wish = Vector3Add(wish, Vector3{ 0.0f, 1.0f, 0.0f });
        }

        if (IronHull::Input::is_action_pressed("move_down")) {
            wish = Vector3Subtract(wish, Vector3{ 0.0f, 1.0f, 0.0f });
        }

        if (Vector3LengthSqr(wish) > 0.0f) {
            wish = Vector3Normalize(wish);
        }

        this->position = Vector3Add(this->position, Vector3Scale(wish, NOCLIP_SPEED * delta));
        this->on_ground = false;
        return;
    }

    // Walking ignores where the player is looking vertically: looking at the floor should
    // not slow them down.
    wish.y = 0.0f;

    if (Vector3LengthSqr(wish) > 0.0f) {
        wish = Vector3Normalize(wish);
    }

    IronHull::TraceResult ground = this->map.trace_box(this->position,
        Vector3Add(this->position, Vector3{ 0.0f, -GROUND_PROBE, 0.0f }),
        PLAYER_MINS, PLAYER_MAXS);

    this->on_ground = ground.hit && ground.normal.y > GROUND_NORMAL;

    // Horizontal velocity is set outright rather than accelerated towards. There is no
    // movement model to speak of here - this scene exists to show the map and collision
    // working, and anything springier would only get in the way of judging that.
    Vector3 horizontal = Vector3Scale(wish, WALK_SPEED);
    this->velocity.x = horizontal.x;
    this->velocity.z = horizontal.z;

    if (this->on_ground) {
        // Held against the floor rather than zeroed, so the ground probe keeps finding it
        // and the player does not drift into a one-frame fall on every step.
        this->velocity.y = -GROUND_PROBE / std::max(delta, 0.0001f);

        if (IronHull::Input::is_action_pressed("jump")) {
            this->velocity.y = JUMP_SPEED;
            this->on_ground = false;
        }
    } else {
        this->velocity.y -= GRAVITY * delta;
    }

    this->move_with_collision(delta);
}

void MapScene::move_with_collision(float delta)
{
    Vector3 remaining = Vector3Scale(this->velocity, delta);

    // Each pass moves as far as it can, then turns what is left to travel along the surface
    // it hit. Four passes is enough to get out of a corner, where the motion has to be
    // folded against two or three walls in turn.
    for (int attempt = 0; attempt < 4; ++attempt) {
        if (Vector3LengthSqr(remaining) < 0.0001f) {
            break;
        }

        Vector3 target = Vector3Add(this->position, remaining);
        IronHull::TraceResult hit = this->map.trace_box(this->position, target, PLAYER_MINS, PLAYER_MAXS);

        // Starting inside something means the player is already embedded - a map change, or
        // a door closing on them. Moving on the trace's terms would teleport them to the
        // start of the sweep, so the move is simply abandoned for this frame.
        if (hit.all_solid) {
            break;
        }

        this->position = hit.position;

        if (!hit.hit) {
            break;
        }

        if (hit.normal.y > GROUND_NORMAL) {
            this->on_ground = true;
        }

        // Project both what is left of the move and the velocity onto the surface, so the
        // player slides along a wall instead of stopping against it, and does not keep
        // building up speed into it.
        Vector3 rest = Vector3Scale(remaining, 1.0f - hit.fraction);
        remaining = Vector3Subtract(rest, Vector3Scale(hit.normal, Vector3DotProduct(rest, hit.normal)));

        this->velocity = Vector3Subtract(this->velocity,
            Vector3Scale(hit.normal, Vector3DotProduct(this->velocity, hit.normal)));
    }
}

void MapScene::update_camera()
{
    this->camera.position = Vector3Add(this->position, Vector3{ 0.0f, EYE_HEIGHT, 0.0f });
    this->camera.target = Vector3Add(this->camera.position, IronHull::MapSpace::angles_to_forward(this->look));
}

void MapScene::on_draw(IronHull::RenderPass pass)
{
    if (pass == IronHull::RenderPass::VIEWPORT) {
        ClearBackground(Color{ 24, 26, 32, 255 });

        if (!this->map_ready) {
            return;
        }

        BeginMode3D(this->camera);
        {
            this->map.draw();

            if (this->wireframe) {
                this->map.draw_debug_wireframe();
            }
        }
        EndMode3D();
    }

    if (pass == IronHull::RenderPass::SCREEN) {
        this->draw_hud();
    }
}

void MapScene::draw_hud()
{
    if (!this->map_ready) {
        DrawText("Could not load the map:", 16, 16, 20, RED);
        DrawText(this->load_error.c_str(), 16, 42, 16, RAYWHITE);
        DrawText("Compile it first:  ihbsp sandbox/maps/testmap.map", 16, 72, 16, GRAY);
        DrawText("The runtime reads content:// from a 'content' directory beside the executable.",
            16, 94, 16, GRAY);
        return;
    }

    unsigned int contents = this->map.point_contents(
        Vector3Add(this->position, Vector3{ 0.0f, EYE_HEIGHT, 0.0f }));

    const char* standing = this->on_ground ? "ground" : "air";
    const char* where = (contents & IronHull::CONTENTS_LIQUID_MASK) != 0u ? "submerged"
        : ((contents & IronHull::CONTENTS_SOLID_MASK) != 0u ? "in solid" : "open");

    DrawText(TextFormat("%2i fps", GetFPS()), 16, 16, 20, RAYWHITE);
    DrawText(TextFormat("pos  %.0f %.0f %.0f", this->position.x, this->position.y, this->position.z),
        16, 42, 18, RAYWHITE);
    DrawText(TextFormat("look  pitch %.0f  yaw %.0f", this->look.x, this->look.y), 16, 64, 18, RAYWHITE);
    DrawText(TextFormat("state  %s, %s%s", standing, where, this->noclip ? ", noclip" : ""),
        16, 86, 18, RAYWHITE);
    DrawText(TextFormat("entities  %i live, %i queued inputs",
        this->map.world().count(), this->map.world().pending_input_count()), 16, 108, 18, RAYWHITE);
    DrawText(TextFormat("faces  %zu   leaves  %zu",
        this->map.compiled().faces.size(), this->map.compiled().leaves.size()), 16, 130, 18, RAYWHITE);

    int height = IronHull::Application::get_viewport().height;
    DrawText("WASD move   Space jump   V noclip   F3 wireframe   Esc release mouse",
        16, height - 30, 18, Color{ 160, 160, 170, 255 });

    // The zombie in the test map rots to death on its own and its OnDied is wired to the
    // door, so the map demonstrates its own I/O without being touched.
    std::vector<IronHull::Entity*> zombies = this->map.world().find_by_classname("npc_zombie");

    if (!zombies.empty()) {
        sol::table self = zombies[0]->lua_table();

        // Read as a double even though the property is declared an int. A Lua number does
        // not keep its subtype through arithmetic - the zombie subtracts a float rot_rate
        // from its health, which turns the value into a float - and asking sol2 for an int
        // when Lua is holding a float aborts the process rather than converting.
        double health = self["props"]["health"].get<double>();

        DrawText(TextFormat("zombie health  %i%s", static_cast<int>(health),
            health <= 0.0 ? "  (door opening)" : ""),
            16, 152, 18, health > 0.0 ? Color{ 200, 230, 160, 255 } : Color{ 230, 160, 160, 255 });
    }
}

void MapScene::on_dispose()
{
    if (this->mouse_captured) {
        EnableCursor();
    }

    this->map.unload();

    // Definitions hold tables belonging to the Lua state, so they have to go first.
    IronHull::EntityRegistry::clear();
    IronHull::LuaVM::shutdown();
}
