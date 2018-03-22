-- Dim effect

config = {
    delay = (tonumber(keyleds.config.delay) or 60) * 1000,  -- in s in config
    fadeIn = tonumber(keyleds.config.fadein) or 300,        -- in ms
    fadeOut = tonumber(keyleds.config.fadeout) or 5000,     -- in ms
    color = tocolor(keyleds.config.color) or tocolor(0, 0, 0, 0.6)
}
transparent = tocolor(0, 0, 0, 0)

-- status values:
-- 0 = inactive
-- 1 = fading out
-- 2 = active (dimmed)
-- 3 = fading in

function init()
    buffer = RenderTarget:new()  -- this will hold key colors
    color = tocolor(config.color.red, config.color.green, config.color.blue, 0)
    status = 0
    timer = 0
end

function render(ms, target)
    timer = timer + ms

    if status == 0 then
        if timer >= config.delay then
            timer = 0
            status = 1
        end
    elseif status == 1 then
        if timer >= config.fadeOut then
            timer = 0
            status = 2
            buffer:fill(config.color)
        else
            color.alpha = config.color.alpha * (timer / config.fadeOut)
            buffer:fill(color)
        end
    elseif status == 3 then
        if timer >= config.fadeIn then
            -- do not reset timer
            status = 0
            buffer:fill(transparent)
        else
            color.alpha = config.color.alpha * (1 - timer / config.fadeIn)
            buffer:fill(color)
        end
    end

    if status ~= 0 then target:blend(buffer) end
end

function onKeyEvent(key, isPress)
    if status == 0 then
        timer = 0
    elseif status == 1 then
        timer = (1 - timer / config.fadeOut) * config.fadeIn
        status = 3
    elseif status == 2 then
        timer = 0
        status = 3
    end
end
