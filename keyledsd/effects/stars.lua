-- Stars effect

function stars(kb)
    -- Read configuration
    local duration = (tonumber(keyleds.config.duration) or 1000) / 1000
    local number = tonumber(keyleds.config.number) or 8
    local keys = keyleds.groups[keyleds.config.group] or keyleds.db

    local colors, transparents = {}, {}

    local colornames
    if keyleds.config.colors then
        colornames = keyleds.config.colors
    else
        colornames = {}
        for key, value in pairs(keyleds.config) do
            if string.sub(key, 1, 5) == "color" then colornames[#colornames + 1] = value end
        end
        print("numbered lists are deprecated, please check https://github.com/spectras/keyleds/wiki/Numbered-list-deprecation for help.")
    end

    for idx, value in ipairs(colornames) do
        local color = tocolor(value)
        if color then
            colors[#colors + 1] = color
            transparents[#transparents + 1] = tocolor(color.red, color.red, color.blue, 0)
        end
    end
    if #colors == 0 then colors[1], transparents[1] = tocolor('white'), tocolor(1, 1, 1, 0) end

    -- Animation loop
    local delay = duration / number
    while true do
        local colornum = math.random(#colors)
        local key = keys[math.random(#keys)]

        kb[key] = fade(duration, colors[colornum], transparents[colornum])
        wait(delay)
    end
end

buffer = RenderTarget:new()
thread(stars, buffer)
function render(ms, target) target:blend(buffer) end
