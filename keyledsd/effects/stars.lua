-- Stars effect

transparent = tocolor(0, 0, 0, 0)

config = {
    duration = tonumber(keyleds.config.duration) or 1000,
    number = tonumber(keyleds.config.number) or 8,
    keys = keyleds.groups[keyleds.config.group] or keyleds.db,
    colors = {}     -- will read it in init()
}

-- Effect initialization

function init()
    local key, value
    for key, value in pairs(keyleds.config) do
        if string.sub(key, 1, 5) == "color" then
            local color = tocolor(value)
            if color then config.colors[#config.colors + 1] = color end
        end
    end
    if #config.colors == 0 then config.colors[1] = tocolor('white') end

    local i
    stars = {}
    for i = 1, config.number do
        local star = Star:new()
        star:rebirth()
        star.age = (i - 1) * config.duration / config.number
        stars[#stars + 1] = star
    end

    buffer = RenderTarget:new()
end

-- Effect rendering

function render(ms, target)
    local idx, star, duration
    duration = config.duration
    for idx, star in ipairs(stars) do
        star.age = star.age + ms
        if star.age >= duration then
            buffer[star.key] = transparent
            star:rebirth()
        end
        buffer[star.key] = tocolor(
            star.color.red, star.color.green, star.color.blue,
            star.color.alpha * (duration - star.age) / duration
        )
    end
    target:blend(buffer)
end

-- Star object

Star = {}
function Star:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    return o
end
function Star:rebirth()
    self.key = config.keys[math.random(#config.keys)]
    self.color = config.colors[math.random(#config.colors)]
    self.age = 0
end
