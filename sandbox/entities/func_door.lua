-- func_door
--
-- A brush entity that slides open and shut. This is the worked example of a BrushClass
-- entity: its geometry comes from the brushes the mapper built it out of, the compiler gives
-- it a model of its own so it can move independently of the level, and moving it is a matter
-- of setting its origin - the renderer and the collision hull both follow.

local Door = {
    -- Built from map brushes rather than placed as a point.
    class = "BrushClass",
    classname = "func_door",

    display = "Door",
    category = "Brush",
    description = "A door or platform that slides between a closed and an open position.",

    base = { "Targetname" },

    editor = {
        color = { 200, 160, 60 },
    },

    properties = {
        move_dir = {
            type = "vector",
            -- Authored Z-up, as the editor works: this is straight up. It arrives already
            -- converted into engine space, so the script can use it directly.
            default = { 0, 0, 1 },
            display = "Move Direction",
            description = "Direction the door travels to open.",
            order = 1,
        },
        move_distance = {
            type = "float",
            default = 128.0,
            display = "Move Distance",
            description = "How far the door travels, in map units.",
            min = 0.0,
            order = 2,
        },
        speed = {
            type = "float",
            default = 100.0,
            display = "Speed",
            description = "Units travelled per second.",
            min = 1.0,
            order = 3,
        },
        wait = {
            type = "float",
            default = -1.0,
            display = "Wait",
            description = "Seconds to stay open before closing again. -1 stays open.",
            order = 4,
        },
        start_open = {
            type = "bool",
            default = false,
            display = "Start Open",
            description = "Spawn already at the open position.",
            order = 5,
        },
    },

    outputs = {
        OnOpen = { description = "Fired the moment the door starts opening." },
        OnFullyOpen = { description = "Fired once the door finishes opening." },
        OnClose = { description = "Fired the moment the door starts closing." },
        OnFullyClosed = { description = "Fired once the door finishes closing." },
    },

    inputs = {
        Open = { description = "Start opening." },
        Close = { description = "Start closing." },
        Toggle = { description = "Open a closed door, close an open one." },
    },
}

function Door:OnSpawn()
    -- Where the brushes were built is the closed position, so the door's travel is an offset
    -- away from an origin of zero rather than an absolute destination.
    self.closed_position = self:get_origin()

    local direction = self.props.move_dir
    if direction:length() > 0.0 then
        direction = direction:normalized()
    else
        -- A door with no direction would never move anywhere and look broken, so fall back
        -- to up rather than silently doing nothing.
        self:log("move_dir is zero, defaulting to up")
        direction = Vector3(0, 1, 0)
    end

    self.open_position = self.closed_position + direction * self.props.move_distance

    self.open = self.props.start_open
    self.progress = self.open and 1.0 or 0.0
    self.close_timer = -1.0

    self:set_origin(self.open and self.open_position or self.closed_position)
end

function Door:OnUpdate(delta)
    -- Progress runs 0 at closed to 1 at open, so one number covers both directions and the
    -- door can be reversed part way without any special handling.
    local travel = self.props.move_distance
    local step = travel > 0.0 and (self.props.speed / travel) * delta or 1.0

    local target = self.open and 1.0 or 0.0

    if self.progress ~= target then
        local was = self.progress

        if self.progress < target then
            self.progress = math.min(self.progress + step, target)
        else
            self.progress = math.max(self.progress - step, target)
        end

        self:set_origin(self.closed_position + (self.open_position - self.closed_position) * self.progress)

        if was ~= self.progress and self.progress == target then
            if self.open then
                self:fire("OnFullyOpen")

                if self.props.wait >= 0.0 then
                    self.close_timer = self.props.wait
                end
            else
                self:fire("OnFullyClosed")
            end
        end

        return
    end

    if self.close_timer >= 0.0 then
        self.close_timer = self.close_timer - delta

        if self.close_timer <= 0.0 then
            self.close_timer = -1.0
            self:Close()
        end
    end
end

function Door:Open()
    if self.open then
        return
    end

    self.open = true
    self.close_timer = -1.0
    self:fire("OnOpen")
end

function Door:Close()
    if not self.open then
        return
    end

    self.open = false
    self.close_timer = -1.0
    self:fire("OnClose")
end

function Door:Toggle()
    if self.open then
        self:Close()
    else
        self:Open()
    end
end

return Door
