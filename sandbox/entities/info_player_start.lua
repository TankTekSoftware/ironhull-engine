-- info_player_start
--
-- Where the player begins the map. The engine reads this one through
-- Map::find_player_start() rather than by spawning behaviour onto it, so the script is
-- almost entirely declaration - but it still has to exist, because the compiler floods the
-- map outwards from the point entities to work out what is inside and what is the void.

local PlayerStart = {
    class = "PointClass",
    classname = "info_player_start",

    display = "Player Start",
    category = "Info",
    description = "The point the player spawns at. A map needs exactly one.",

    base = { "Targetname", "Angles" },

    editor = {
        -- A standing player: 32 units across and 72 tall, measured from the floor.
        size = { -16, -16, 0, 16, 16, 72 },
        color = { 80, 160, 255 },
    },

    properties = {},

    outputs = {
        OnPlayerSpawn = { description = "Fired once the player has been placed here." },
    },

    inputs = {},
}

function PlayerStart:OnSpawn()
    self:fire("OnPlayerSpawn")
end

return PlayerStart
