-- Toggle demonstration effect
--
-- This effect demonstrates the basics of making a lua effect. It will:
--  1) read a color from its configuration
--  2) when a key is pressed, toggle the color on and off
--  3) accept a reset event through DBus that blanks everything again

config = {
    color = tocolor(keyleds.config.color) or tocolor('white')
}
transparent = tocolor(0, 0, 0, 0)

-- Script initialization, called by keyleds right after script is loaed

function init()
    keyStates = {}                      -- this will hold one boolean per key: on/off
    buffer = RenderTarget:new()  -- this will hold key colors
end

-- Effect rendering, called by keyleds several times per second to update leds

function render(ms, target)
    -- ms is the number of milliseconds between calls (useful for animations)
    -- target is where to draw into, it's the same kind of object as our buffer...

    -- ... and we can simply blend ours into it
    target:blend(buffer)
end

-- Keyboard event, called by keyleds when a key is pressed or released

function onKeyEvent(key, isPress)
    -- key is the keyleds.db entry of the key
    -- isPress is true if they key was just pressed, false if it was just released

    if not isPress then return end

    local state = not keyStates[key]
    keyStates[key] = state
    if state then
        buffer[key] = config.color
    else
        buffer[key] = transparent
    end
end

-- Generic event, called by keyleds when requested to through DBus

function onGenericEvent(data)
    -- data is the dictionary passed through DBus event
    if data.effect ~= "toggle" then return end  -- we only handle the event if it says effect=toggle
                                                -- not mandatory, but a good practice to isolate events

    if data.command == "reset" then             -- when command=reset, clear all states
        for key, value in pairs(keyStates) do
            keyStates[key] = false
            buffer[key] = transparent
        end
    end
end

-- The reset can be tested from the command line with the following command:
--
-- dbus-send --session --dest=org.etherdream.KeyledsService \
--           /Service org.etherdream.keyleds.Service.sendGenericEvent \
--           dict:string:string:"effect","toggle","command","reset"
--
-- Or from python (see the wiki if you need help with pydbus setup):
-- import pydbus
-- service = pydbus.SessionBus().get('org.etherdream.KeyledsService', '/Service')
-- service.sendGenericEvent({"effect": "toggle", "command": "reset"})
