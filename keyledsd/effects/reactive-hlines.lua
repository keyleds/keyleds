-- Data

lineDefs = {
    {"ESC", "F1", "F2", "F3", "F4", "F5", "F6", "F7",
     "F8", "F9", "F10", "F11", "F12", "SYSRQ", "SCROLLLOCK", "PAUSE"},
    {"GRAVE", "1", "2", "3", "4", "5", "6", "7",
     "8", "9", "0", "MINUS", "EQUAL", "BACKSPACE", "INSERT", "HOME",
     "PAGEUP", "NUMLOCK", "KPSLASH", "KPASTERISK", "KPMINUS"},
    {"TAB", "Q", "W", "E", "R", "T", "Y", "U",
     "I", "O", "P", "LBRACE", "RBRACE", "ENTER", "DELETE", "END",
     "PAGEDOWN", "KP7", "KP8", "KP9", "KPPLUS"},
    {"CAPSLOCK", "A", "S", "D", "F", "G", "H", "J",
     "K", "L", "SEMICOLON", "APOSTROPHE", "BACKSLASH", "ENTER", 0, 0,
     0, "KP4", "KP5", "KP6", "KPPLUS"},
    {"LSHIFT", "102ND", "Z", "X", "C", "V", "B", "N",
     "M", "COMMA", "DOT", "SLASH", "RSHIFT", 0, "UP", 0,
     "KP1", "KP2", "KP3", "KPENTER"},
    {"LCTRL", "LMETA", "LALT", "SPACE", "RALT", "FN", "COMPOSE", "RCTRL",
     "LEFT", "DOWN", "RIGHT", "KP0", "KPDOT", "KPENTER"}
}

color = tocolor(keyleds.config.color) or tocolor('white')
transparent = tocolor(color.red, color.green, color.blue, 0)
speed = tonumber(keyleds.config.speed) or 0.02
fadeTime = tonumber(keyleds.config.fadeTime) or 0.5

-- Lookup all names into key entries, eliminating unavailable ones
function initLines()
    lines = {}
    for i, lineDef in ipairs(lineDefs) do
        local line = {}
        for j, def in ipairs(lineDef) do
            if def == 0 then
                line[#line + 1] = 0
            else
                local key = keyleds.db:findName(def)
                if key then line[#line + 1] = key end
            end
        end
        lines[#lines + 1] = line
    end
end

-- Return (line, index) of given key within lines table
function findKey(which)
    for i, line in ipairs(lines) do
        for j, key in ipairs(line) do
            if key == which then return line, j end
        end
    end
    return nil
end

-- Play animation for given key
function react(kb, key)
    local line, idx = findKey(key)
    if not line then print("Key ", key, " not found") return end

    kb[line[idx]] = fade(fadeTime, color, transparent)
    for i = 1, math.max(idx - 1, #line - idx) do
        wait(speed)
        if idx - i > 0 and line[idx - i] ~= 0 then
            kb[line[idx - i]] = fade(fadeTime, color, transparent)
        end
        if idx + i <= #line and line[idx + i] ~= 0 then
            kb[line[idx + i]] = fade(fadeTime, color, transparent)
        end
    end
end

-- Weak table of active animations and their buffer, used for rendering
buffers = {}
setmetatable(buffers, { __mode = 'kv' })

-- Invoked whenever a key is pressed or release; trigger the animation
function onKeyEvent(key, isPress)
    if not isPress then return end
    buffer = RenderTarget:new()
    local thread = thread(react, buffer, key)
    buffers[thread] = buffer
end

-- Blend all ongoing animations into the target
function render(ms, target)
    for thread, buffer in pairs(buffers) do target:blend(buffer) end
end

-- Don't forget running init code
initLines()
