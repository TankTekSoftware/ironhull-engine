-- light
--
-- A point light. There is no lighting system in the engine yet, so nothing reads these at
-- draw time - the renderer shades map surfaces from a fixed direction instead. The entity
-- is here so maps can be lit now and have that lighting mean something later, and because
-- it is the simplest thing with a switchable state to wire a trigger up to.

local Light = {
    class = "PointClass",
    classname = "light",

    display = "Light",
    category = "Lighting",
    description = "A point light. Switchable through its Toggle input.",

    base = { "Targetname" },

    editor = {
        size = { -8, -8, -8, 8, 8, 8 },
        color = { 255, 220, 120 },
        icon = "icons/light.png",
    },

    properties = {
        brightness = {
            type = "float",
            default = 1.0,
            display = "Brightness",
            description = "Multiplier on the light's output.",
            min = 0.0,
            order = 1,
        },
        light_color = {
            type = "color",
            default = { 255, 240, 210 },
            display = "Colour",
            description = "Colour the light casts.",
            order = 2,
        },
        radius = {
            type = "float",
            default = 256.0,
            display = "Radius",
            description = "How far the light reaches, in map units.",
            min = 0.0,
            order = 3,
        },
        start_off = {
            type = "bool",
            default = false,
            display = "Start Off",
            description = "Spawn dark until switched on.",
            order = 4,
        },
    },

    outputs = {
        OnTurnedOn = { description = "Fired when the light comes on." },
        OnTurnedOff = { description = "Fired when the light goes out." },
    },

    inputs = {
        TurnOn = { description = "Switch the light on." },
        TurnOff = { description = "Switch the light off." },
        Toggle = { description = "Switch the light to whichever state it is not in." },
        SetBrightness = {
            description = "Set the brightness multiplier.",
            parameters = {
                { type = "float", display = "Brightness" }
            },
        },
    },
}

function Light:OnSpawn()
    self.on = not self.props.start_off
end

function Light:TurnOn()
    if self.on then
        return
    end

    self.on = true
    self:fire("OnTurnedOn")
end

function Light:TurnOff()
    if not self.on then
        return
    end

    self.on = false
    self:fire("OnTurnedOff")
end

function Light:Toggle()
    if self.on then
        self:TurnOff()
    else
        self:TurnOn()
    end
end

function Light:SetBrightness(brightness)
    self.props.brightness = math.max(brightness, 0.0)
end

return Light
