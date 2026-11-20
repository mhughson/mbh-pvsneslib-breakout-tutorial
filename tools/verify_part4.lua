-- Mesen 2 integration checks. Run with a separate emulator profile.
local stage = tonumber((assert(os.getenv("PART4_STAGE"), "PART4_STAGE missing")))
local output = assert(os.getenv("PART4_OUTPUT"), "PART4_OUTPUT missing")
local symbols = {}
do
    local file = assert(io.open(assert(os.getenv("PART4_SYMBOLS")), "r"))
    for line in file:lines() do
        local address, name = line:match("^(%x+) (%S+)$")
        if address then symbols[name] = tonumber(address, 16) end
    end
    file:close()
end

local ram = emu.memType.snesWorkRam
local vram = emu.memType.snesVideoRam
local held = {}
local first_log = true
local function log(message)
    local file = assert(io.open(output .. "/stage" .. stage .. ".log",
                               first_log and "w" or "a"))
    first_log = false
    file:write(message .. "\n")
    file:close()
end
log("Starting")
local function address(name)
    return assert(symbols[name] or symbols["tccs_src/main.asm_" .. name],
                  "Missing symbol: " .. name) & 0x1FFFF
end
local function read16(at, kind)
    kind = kind or ram
    return emu.read(at, kind) + 256 * emu.read(at + 1, kind)
end
local function write16(at, value)
    emu.write(at, value & 255, ram)
    emu.write(at + 1, (value >> 8) & 255, ram)
end
local game = address("game")
local function get(offset) return read16(game + offset) end
local function put(offset, value) write16(game + offset, value) end
local function variable(name) return read16(address(name)) end
local function wait(frames)
    for _ = 1, frames do coroutine.yield() end
end
local function until_true(predicate, frames, message)
    for _ = 1, frames do
        if predicate() then return end
        wait(1)
    end
    assert(false, message)
end
local function tap(button)
    held[button] = true
    wait(3)
    held[button] = nil
    wait(3)
end
local function shot(name)
    local file = assert(io.open(output .. "/" .. name .. ".png", "wb"))
    file:write(emu.takeScreenshot())
    file:close()
end
local function brick_offset()
    if stage <= 1 then return 20 end
    if stage <= 4 then return 22 end
    if stage == 5 then return 26 end
    if stage == 6 then return 46 end
    return 50
end
local function capsule_offset() return stage == 6 and 28 or 32 end
local function brick(index) return emu.read(game + brick_offset() + index, ram) end
local function set_brick(index, health)
    emu.write(game + brick_offset() + index, health, ram)
end
local function velocity(x, y)
    local unit = stage >= 5 and 16 or 1
    put(6, x * unit)
    put(8, y * unit)
    if stage >= 5 then
        put(22, 0)
        put(24, 0)
    end
end
local function start_game()
    until_true(function()
        return variable("brightness") == 15 and variable("fade_step") == 0
    end, 180, "Title fade did not finish")
    tap("start")
    until_true(function() return variable("screen") == 1 end, 100, "No game screen")
    if stage >= 3 then
        until_true(function() return variable("entrance_offset") > 0 end,
                   100, "Entrance did not start")
        until_true(function() return variable("entrance_offset") == 0 end,
                   100, "Entrance did not finish")
    end
    until_true(function()
        return variable("brightness") == 15 and variable("fade_step") == 0
    end, 100, "Fade did not finish")
    wait(10)
    assert(get(16) == 0, "Entrance launched the ball")
end

local function run()
    wait(160)
    assert(variable("screen") == 0 and variable("brightness") == 15,
           "Title did not boot")
    if stage == 9 then
        assert(variable("high_score") == 1460, "Saved record did not reload")
        return
    end
    local scroll = variable("backdrop_scroll")
    wait(16)
    assert(variable("backdrop_scroll") == ((scroll + 16) & 1023),
           "Title scroll did not advance")
    if stage == 1 then
        shot("part4-section1-scroll")
        start_game()
        assert(variable("backdrop_scroll") == 0, "Backdrop did not reset")
        return
    end
    if stage == 3 then
        tap("start")
        until_true(function() return variable("screen") == 1 end, 100, "No entrance")
        until_true(function() return variable("entrance_offset") > 0 end,
                   100, "Entrance did not start")
        local paddle = get(0)
        held.right, held.b = true, true
        until_true(function() return variable("entrance_offset") <= 112 end,
                   100, "Entrance stalled")
        assert(get(0) == paddle and get(16) == 0, "Gameplay ran during entrance")
        shot("part4-section3-entrance")
        until_true(function() return variable("entrance_offset") == 0 end,
                   100, "Entrance did not finish")
        wait(3)
        assert(get(16) == 0, "Held launch escaped the entrance gate")
        held.right, held.b = nil, nil
        for word = 0x6400, 0x67FF do
            assert(read16(word * 2, vram) == 0x0800, "Blank map half was overwritten")
        end
        return
    end
    start_game()
    assert(get(14) == 56 and get(20) == 0, "Wrong first level")
    if stage == 2 then
        shot("part4-section2-levels")
        return
    end
    if stage == 4 then
        -- WRAM fixture: hit a two-hit top brick from the open area above it.
        put(16, 1)
        put(2, 52)
        put(4, 40)
        velocity(0, 3)
        wait(4)
        assert(brick(2) == 1 and get(10) == 0 and get(14) == 56,
               "Damage scored or removed a brick")
        velocity(0, 0)
        wait(12)
        shot("part4-section4-tough-bricks")
        local tile = read16((0x6000 + 6 * 32 + 6) * 2, vram)
        local weak = read16((0x6000 + 10 * 32 + 6) * 2, vram)
        assert(tile == weak, "Damaged brick did not change graphics")
        return
    end
    if stage == 5 then
        -- WRAM fixture: a center paddle catch chooses the fractional angle.
        put(16, 1)
        put(2, get(0) + 20)
        put(4, 192)
        velocity(0, 3)
        wait(4)
        assert(get(6) == 8 and get(8) == 65536 - 48, "Wrong center angle")
        wait(30)
        shot("part4-section5-angles")
        return
    end
    if stage == 6 then
        -- WRAM fixture: the next destroyed bottom-row brick is drop number four.
        put(16, 1)
        put(26, 3)
        put(2, 116)
        put(4, 88)
        velocity(0, -3)
        wait(4)
        assert(get(capsule_offset() + 4) == 1, "No capsule spawned")
        wait(14)
        shot("part4-section6-capsules")
        return
    end
    if stage == 7 then
        -- WRAM fixture: collect a capsule, with the ball safely stationary.
        put(16, 1)
        put(2, 80)
        put(4, 140)
        velocity(0, 0)
        local capsule = capsule_offset()
        put(capsule, get(0))
        put(capsule + 2, 192)
        put(capsule + 4, 1)
        wait(10)
        assert(get(28) == 64 and get(30) > 0, "Capsule did not widen the paddle")
        shot("part4-section7-wide")
        tap("start")
        assert(variable("paused") == 1, "Pause failed")
        local timer = get(30)
        wait(90)
        assert(get(30) == timer, "Pause spent the power-up timer")
        tap("start")
        until_true(function() return get(30) == 0 end, 650, "Wide effect never expired")
        assert(get(28) == 48, "Paddle did not shrink")
        return
    end
    write16(address("high_score"), 0)
    for level = 0, 2 do
        -- WRAM fixture: each level's last brick and preceding campaign score.
        for index = 0, 69 do set_brick(index, 0) end
        set_brick(34, 1)
        put(10, ({550, 910, 1450})[level + 1])
        put(14, 1)
        put(16, 1)
        put(2, 116)
        put(4, 72)
        velocity(0, -3)
        wait(15)
        if level < 2 then
            assert(get(16) == 4, "Missing level-clear state")
            assert(variable("high_score") == 0, "Interlude saved a final record")
            local score, lives = get(10), get(12)
            tap("b")
            until_true(function() return variable("entrance_offset") > 0 end,
                       100, "Next-level entrance did not start")
            assert(get(20) == level + 1, "Next-level index did not advance")
            until_true(function() return variable("entrance_offset") == 0 end,
                       100, "Next-level entrance stalled")
            wait(10)
            assert(get(16) == 0 and get(10) == score and get(12) == lives,
                   "Next level reset score/lives or launched the ball")
            local count = 0
            for row = 6, 10 do
                for col = 2, 29, 2 do
                    if (read16((0x6000 + row * 32 + col) * 2, vram) & 0x3FF) ~= 0 then
                        count = count + 1
                    end
                end
            end
            assert(count == (level == 0 and 36 or 54), "Wrong uploaded level map")
        end
    end
    assert(get(16) == 3 and get(10) == 1460, "Final brick did not win the campaign")
    assert(variable("high_score") == 1460, "Record was not saved")
    shot("part4-section8-victory")
    tap("b")
    until_true(function() return variable("screen") == 0 end, 100, "No return to title")
    wait(25)
    assert(variable("entrance_offset") == 0, "Title retained gameplay scroll")
    start_game()
    assert(get(10) == 0 and get(12) == 3 and get(20) == 0, "New game did not reset")
    -- WRAM fixture: a lower-scoring last-life miss must not replace the record.
    put(16, 1)
    put(12, 1)
    put(4, 223)
    velocity(0, 3)
    wait(15)
    assert(get(16) == 2 and variable("high_score") == 1460,
           "Lower score replaced the saved record")
end

local co = coroutine.create(run)
emu.addEventCallback(function()
    if coroutine.status(co) == "dead" then return end
    local ok, err = coroutine.resume(co)
    if not ok then
        log("FAIL: stage " .. stage .. ": " .. tostring(err))
        emu.stop(1)
    elseif coroutine.status(co) == "dead" then
        log("PASS: stage " .. stage)
        emu.stop(0)
    end
end, emu.eventType.endFrame)
emu.addEventCallback(function()
    local buttons = {}
    for _, name in ipairs({"a", "b", "x", "y", "l", "r", "up", "down",
                          "left", "right", "select", "start"}) do
        buttons[name] = held[name] == true
    end
    emu.setInput(buttons, 0, 0)
end, emu.eventType.inputPolled)
