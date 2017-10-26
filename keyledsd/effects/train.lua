-- Stars effect

transparent = tocolor(0, 0, 0, 0)

path = {
    "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8",
    "F9", "F10", "F11", "F12", "BACKSPACE", "EQUAL", "MINUS", "0",
    "9", "8", "7", "6", "5", "4", "3", "2",
    "1", "TAB", "Q", "W", "E", "R", "T", "Y",
    "U", "I", "O", "P", "LBRACE", "RBRACE", "APOSTROPHE", "SEMICOLON",
    "L", "K", "J", "H", "G", "F", "D", "S",
    "A", "102ND", "Z", "X", "C", "V", "B", "N",
    "M", "COMMA", "DOT", "SLASH", "RSHIFT", "RCTRL", "COMPOSE", "FN",
    "RALT", "SPACE", "LALT", "LMETA", "LCTRL"
}

function train(buffer, color, delay)
    local i, key
    wait(delay)
    while true do
        for i, key in ipairs(path) do
            key = keyleds.db:findName(key)
            buffer[key] = color
            wait(0.250)
            buffer[key] = transparent
        end
        wait(2)
    end
end

buffer = RenderTarget:new()
thread(train, buffer, tocolor(0, 1, 0, 1), 0)
thread(train, buffer, tocolor(1, 0, 0, 1), 3)
thread(train, buffer, tocolor(0, 0, 1, 1), 6)

function render(ms, target) target:blend(buffer) end
