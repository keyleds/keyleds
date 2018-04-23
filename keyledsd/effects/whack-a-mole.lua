
config = {
    moleIntervalIncrement = .95,
    moleLifetimeIncrement = .98,
    startColor = keyleds.config.startColor or tocolor(.5, .5, 0),
    endColor = keyleds.config.endColor or tocolor(1, 0, 0),

    keys = keyleds.groups[keyleds.config.group] or keyleds.db
}

scoreKeys = {"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12"}
scoreColors = {tocolor(0, 1, 0), tocolor(1, .8, 0), tocolor(1,0, 0)}

transparent = tocolor(0, 0, 0, 0)


-- Game cycle

function init()
    buffer = RenderTarget:new()
    initGame()
end

function initGame()
    print("New Game")
    moles = {}
    successes, failures = 0, 0
    moleInterval = 2000
    nextMole = 1000
    moleLifetime = 3000
    buffer:fill(transparent)
    renderScore(failures)
end

function renderScore(score)
    local color
    local baseColor = math.floor(score / #scoreKeys) + 1
    local baseScore = score - (score % #scoreKeys)

    for index, key in ipairs(scoreKeys) do
        if index <= score - baseScore then
            color = scoreColors[baseColor + 1]
        else
            color = scoreColors[baseColor]
        end
        buffer[key] = color
    end
end

function handleSuccess()
    successes = successes + 1
    if failures > 0 then failures = failures - 1 end
    moleInterval = 500 * (config.moleIntervalIncrement^math.sqrt(successes))
    moleLifetime = 3000 * (config.moleLifetimeIncrement^math.sqrt(successes))
    renderScore(failures)
    print("score: ", successes)
end

function handleFailure()
    failures = failures + 1
    renderScore(failures)
end

-- Mole updates

function makeMole(lifetime)
    local mole = {}
    mole.key = config.keys[math.random(#config.keys)]
    mole.age = 0
    mole.lifetime = lifetime
    return mole
end

function updateMoles(ms)
    local mole, age, ratio
    for index = #moles, 1, -1 do
        mole = moles[index]
        age = mole.age + ms
        if age < mole.lifetime then
            ratio = age / mole.lifetime
            buffer[mole.key] = config.startColor * (1 - ratio) + config.endColor * ratio
            mole.age = age
        else
            killMole(index)
            handleFailure()
        end
    end
end

function killMole(index)
    local mole = moles[index]
    buffer[mole.key] = transparent
    table.remove(moles, index)
end

-- Events

function render(ms, target)
    updateMoles(ms)
    if failures >= 2 * #scoreKeys then
        initGame()
    end
    if nextMole > ms then
        nextMole = nextMole - ms
    else
        local newMole = makeMole(moleLifetime)
        table.insert(moles, newMole)
        buffer[newMole.key] = config.startColor
        nextMole = moleInterval
    end
    target:blend(buffer)
end

function onKeyEvent(key, isPress)
    if not isPress then return end

    for index, mole in ipairs(moles) do
        if mole.key == key then
            killMole(index)
            handleSuccess()
            return
        end
    end
    handleFailure()
end

function onContextChange(context)
    initGame()
end
