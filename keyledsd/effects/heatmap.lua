
cold = tocolor(keyleds.config.cold) or tocolor('blue')
hot = tocolor(keyleds.config.hot) or tocolor('red')
transparent = tocolor(0, 0, 0, 0)

function init()
    contexts = {}
    currentId, presses, maximum = "\0", {}, 0
    buffer = RenderTarget:new()
end

function renderToBuffer(buffer, presses, maximum)
    buffer:fill(transparent)
    for key, count in pairs(presses) do
        local ratio = count / maximum
        buffer[key] = hot * ratio + cold * (1 - ratio)
    end
end

function onKeyEvent(key, isPress)
    if not isPress then return end

    local count = (presses[key] or 0) + 1
    presses[key] = count
    if count > maximum then maximum = count end

    renderToBuffer(buffer, presses, maximum)
end

function onContextChange(context)
    local id = (context.class or "") .. "\0" .. (context.instance or "")
    if id == currentId then return end

    -- save current context
    contexts[currentId] = {presses, maximum}

    -- load context
    local stored = contexts[id]
    if stored then
        presses, maximum = stored[1], stored[2]
    else
        presses, maximum = {}, 0
    end
    currentId = id

    -- update buffer
    renderToBuffer(buffer, presses, maximum)
end

function onGenericEvent(data)
    if data.effect ~= "heatmap" then return end
    if data.command == "reset" then init() end
end

function render(ms, target)
    target:blend(buffer)
end
