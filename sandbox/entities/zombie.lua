-- npc_zombie
--
-- An entity script is a single table describing the entity to the engine and
-- the editor, plus the methods that give it behaviour. The table is returned at
-- the bottom of the file; the engine reads the declarative fields to build the
-- editor entity definition (this is what an FGD would have given us) and calls
-- the methods at runtime.

local Zombie = {
    -- How the editor is allowed to place this entity:
    --   "PointClass" - placed as a point in the world, uses editor.size/model.
    --   "BrushClass" - built from map brushes, geometry comes from the solid.
    class = "PointClass",

    -- Name written into the map file, and the name used to spawn at runtime.
    classname = "npc_zombie",

    -- Shown in the editor's entity browser and inspector header.
    display = "Zombie",
    category = "NPC",
    description = "A slow melee NPC that slowly rots away while alive.",

    -- Shared property sets mixed in before this script's own properties.
    base = { "Targetname", "Angles" },

    -- Editor-only presentation. Ignored at runtime.
    editor = {
        -- Selection/render bounds as { min_x, min_y, min_z, max_x, max_y, max_z }.
        size = { -16, -16, 0, 16, 16, 72 },
        color = { 120, 180, 90 },
        model = "models/npc/zombie.obj",
        icon = "icons/npc_zombie.png",
    },

    -- Public properties. Every key here is exposed in the inspector and can be
    -- overridden per-instance in the map; the resolved values land on
    -- self.props at spawn time.
    --
    -- type is one of: int, float, bool, string, vector, angle, color,
    -- choices, flags, target, model, material, sound.
    -- order is optional; properties without one sort after those with one.
    properties = {
        health = {
            type = "int",
            default = 100,
            display = "Health",
            description = "Hit points the zombie spawns with.",
            min = 1,
            order = 1,
        },
        rot_rate = {
            type = "float",
            default = 1.0,
            display = "Rot Rate",
            description = "Hit points lost per second while alive.",
            min = 0.0,
            order = 2,
        },
        move_speed = {
            type = "float",
            default = 48.0,
            display = "Move Speed",
            description = "Units travelled per second when chasing.",
            min = 0.0,
            order = 3,
        },
        corpse = {
            type = "choices",
            default = "linger",
            display = "On Death",
            description = "What to do with the body once killed.",
            choices = {
                { value = "linger", display = "Leave corpse" },
                { value = "fade", display = "Fade out" },
                { value = "remove", display = "Remove immediately" },
            },
            order = 4,
        },
        start_asleep = {
            type = "bool",
            default = false,
            display = "Start Asleep",
            description = "Spawn dormant until the Wake input is fired.",
            order = 5,
        },
    },

    -- Events this entity fires. The editor lists these as the left-hand side of
    -- a map I/O connection; fire them with self:fire(name).
    outputs = {
        OnSpawn = { description = "Fired once the zombie finishes spawning." },
        OnDied = { description = "Fired when health reaches zero." },
    },

    -- Events this entity accepts. Each key must have a matching method below;
    -- the editor lists these as the right-hand side of a connection.
    inputs = {
        Damage = {
            description = "Subtract the given amount of health.",
            parameters = {
                { type = "int", display = "Amount" }
            },
        },
        Kill = { description = "Kill the zombie immediately." },
        Wake = { description = "Wake a zombie that spawned asleep." },
    },
}

-- Called once when the entity spawns. Per-instance state belongs on self, not
-- in file-level locals -- every instance of this entity shares the same chunk.
function Zombie:OnSpawn()
    self.rot_timer = 0.0
    self.awake = not self.props.start_asleep
    self.dead = false

    self:fire("OnSpawn")
end

-- Called every frame with the seconds elapsed since the last frame.
function Zombie:OnUpdate(delta)
    if self.dead or not self.awake then
        return
    end

    self.rot_timer = self.rot_timer + delta
    while self.rot_timer >= 1.0 do
        self.rot_timer = self.rot_timer - 1.0
        self:Damage(self.props.rot_rate)
    end
end

function Zombie:Damage(amount)
    if self.dead then
        return
    end

    self.props.health = self.props.health - amount
    if self.props.health <= 0 then
        self.props.health = 0
        self.dead = true

        self:fire("OnDied")
    end
end

function Zombie:Kill()
    self:Damage(self.props.health)
end

function Zombie:Wake()
    self.awake = true
end

return Zombie
