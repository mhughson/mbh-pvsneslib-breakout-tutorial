-- Mesen 2 checks and native-frame captures for Parts 1-3.
-- Use a separate portable profile; fixtures intentionally modify game WRAM.
local part = tonumber((assert(os.getenv("TUTORIAL_PART"))))
local section = tonumber((assert(os.getenv("TUTORIAL_SECTION"))))
local output = assert(os.getenv("TUTORIAL_OUTPUT"))
local symbols = {}
local file = assert(io.open(assert(os.getenv("TUTORIAL_SYMBOLS")), "r"))
for line in file:lines() do
    local value, name = line:match("^(%x+) (%S+)$")
    if value then symbols[name] = tonumber(value, 16) end
end
file:close()

local ram = emu.memType.snesWorkRam
local video = emu.memType.snesVideoRam
local sprites = emu.memType.snesSpriteRam
local buttons = {}
local mouse = { xOffset = 0, yOffset = 0, left = false, right = false }
local function symbol(name)
    return symbols[name] or symbols["tccs_src/main.asm_" .. name]
end
local function address(name)
    return assert(symbol(name), "Missing symbol " .. name) & 0x1FFFF
end
local function read(at, memory)
    memory = memory or ram
    return emu.read(at, memory) + 256 * emu.read(at + 1, memory)
end
local function write(at, value)
    emu.write(at, value & 255, ram)
    emu.write(at + 1, (value >> 8) & 255, ram)
end
local function value(name) return read(address(name)) end
local function wait(count)
    for _ = 1, count do coroutine.yield() end
end
local function await(predicate, count, message)
    for _ = 1, count do
        if predicate() then return end
        wait(1)
    end
    error(message)
end
local function tap(button)
    buttons[button] = true
    wait(3)
    buttons[button] = nil
    wait(3)
end
local function mouse_click(button)
    mouse[button] = true
    wait(3)
    mouse[button] = false
    wait(3)
end
local function capture(label)
    local image = assert(io.open(output .. "/" .. label .. ".png", "wb"))
    image:write(emu.takeScreenshot())
    image:close()
end
local game = symbol("game") and address("game")
local fields = {}
local brick_offset
if game then
    local header = assert(io.open(assert(os.getenv("TUTORIAL_HEADER")), "r"))
    local contents = header:read("*a")
    header:close()
    local structure = assert(contents:match("typedef struct Game%s*{(.-)}%s*Game"))
    local offset = 0
    for line in structure:gmatch("[^\n]+") do
        local name = line:match("^%s*(.-);")
        if name then
            if name:find("bricks%[") then
                brick_offset = offset
            else
                local field = assert(name:match("([%w_]+)$"), "Unexpected game member")
                fields[field] = offset
                offset = offset + 2
            end
        end
    end
end
local canonical_fields = {
    [0] = "paddle_x", [2] = "ball_x", [4] = "ball_y", [6] = "vx",
    [8] = "vy", [10] = "score", [12] = "lives", [14] = "bricks_left", [16] = "state",
}
local function field_address(offset)
    local name = assert(canonical_fields[offset], "Unknown field offset")
    return assert(game) + assert(fields[name], "Missing game member " .. name)
end
local function get(offset) return read(field_address(offset)) end
local function put(offset, v) write(field_address(offset), v) end
local function start()
    if part == 3 and section >= 2 then
        if section >= 4 then
            await(function() return value("fade_step") == 0 end, 200, "Boot fade stalled")
        end
        tap("start")
        await(function() return value("screen") == 1 end, 200, "No gameplay screen")
        if section >= 4 then
            await(function() return value("fade_step") == 0 end, 200, "Game fade stalled")
        end
        wait(15)
    end
end
local function map_bricks()
    local count = 0
    for row = 6, 10 do
        for col = 2, 29, 2 do
            if (read((0x6C00 + row * 32 + col) * 2, video) & 0x3FF) ~= 0 then
                count = count + 1
            end
        end
    end
    return count
end
local function audio()
    local state = emu.getState()
    local total = 0
    for voice = 0, 7 do
        total = total + (state["spc.dsp.voices[" .. voice .. "].envVolume"] or 0)
    end
    return total
end
local function paddle_hit()
    -- Fixture: a descending ball touches the paddle on the next tick.
    put(16, 1)
    put(2, get(0) + 20)
    put(4, 192)
    put(6, 0)
    put(8, 3)
    wait(4)
    assert(get(8) > 32767, "Paddle did not rebound the ball")
end
local function lose_last_life()
    -- Fixture: miss one last life; the game performs the state transition.
    put(16, 1)
    put(12, 1)
    put(2, 16)
    put(4, 223)
    put(6, 0)
    put(8, 3)
    await(function() return get(16) == 2 end, 30, "Life loss did not end the game")
end
local function win()
    -- Fixture: leave the bottom-left brick and accumulated prior score.
    local count = part == 1 and 50 or 70
    local cols = part == 1 and 10 or 14
    local offset = assert(brick_offset, "Missing brick array")
    for index = 0, count - 1 do
        emu.write(game + offset + index, index == 4 * cols and 1 or 0, ram)
    end
    put(14, 1)
    put(10, (count - 1) * 10)
    put(16, 1)
    put(2, part == 1 and 34 or 20)
    put(4, part == 1 and 120 or 88)
    put(6, 0)
    put(8, 65533)
    await(function() return get(16) == 3 end, 30, "Last brick did not win")
    assert(get(10) == count * 10, "Final score is incorrect")
    wait(20)
end
local descriptions = {
    [1] = {"text", "paddle", "launch", "walls", "paddle-bounce",
           "bricks", "lives", "hud", "complete"},
    [2] = {"backdrop", "playfield", "bricks-disappear", "dynamic-sprites",
           "glimmer", "music", "sound-effects", "complete"},
    [3] = {"metasprite", "text-title", "logo", "fade", "gradient",
           "mosaic", "pause", "high-score", "mouse", "complete"}
}
local function run()
    wait(160)
    local label = "part" .. part .. "-section" .. section .. "-"
                  .. assert(descriptions[part][section])
    if part == 1 then
        if section == 2 then
            buttons.left = true
            wait(60)
            buttons.left = nil
            wait(2)
            assert(get(0) == 16, "Paddle did not stop at the left wall")
        elseif section == 3 then
            tap("b")
            assert(get(16) == 1, "Ball did not launch")
        elseif section == 4 then
            tap("b")
            local x, y = get(2), get(4)
            wait(15)
            assert(get(2) ~= x or get(4) ~= y, "Ball did not move")
        elseif section == 5 then
            paddle_hit()
        elseif section == 6 then
            put(16, 1)
            put(2, 34)
            put(4, 120)
            put(6, 0)
            put(8, 65533)
            wait(6)
            assert(get(14) < 50, "Brick did not break")
        elseif section == 7 or section == 8 then
            lose_last_life()
            wait(15)
        elseif section == 9 then
            win()
        end
    elseif part == 2 then
        if section == 2 then
            assert(get(14) == 70, "Wrong new grid count")
        elseif section == 3 then
            put(16, 1)
            put(2, 20)
            put(4, 88)
            put(6, 0)
            put(8, 65533)
            wait(12)
            assert(get(14) == 69 and map_bricks() == 69, "Broken brick remained in VRAM")
        elseif section == 5 then
            paddle_hit()
            assert(value("paddle_glimmer") > 0, "No paddle glimmer")
        elseif section == 6 or section == 7 then
            local heard = false
            for _ = 1, 120 do
                heard = heard or audio() > 0
                wait(1)
            end
            assert(heard, "Music did not activate DSP voices")
            if section == 7 then paddle_hit() end
        elseif section == 8 then
            win()
        end
    elseif part == 3 then
        if section == 4 then
            tap("start")
            await(function()
                return value("fade_step") == 65535 and value("brightness") <= 8
            end, 100, "No fade-out")
        elseif section == 5 then
            start()
        elseif section == 6 then
            start()
            put(16, 1)
            put(4, 223)
            put(6, 0)
            put(8, 3)
            await(function() return value("mosaic_timer") > 0 end, 50, "No life-loss mosaic")
            wait(8)
        elseif section == 7 then
            start()
            tap("b")
            tap("start")
            assert(value("paused") == 1, "Pause did not engage")
            local x, y = get(2), get(4)
            wait(20)
            assert(get(2) == x and get(4) == y, "Paused ball moved")
        elseif section == 8 then
            start()
            put(10, 120)
            lose_last_life()
            wait(35)
            assert(value("high_score") >= 120, "High score was not recorded")
            tap("b")
            await(function() return value("screen") == 0 end, 150, "No title return")
            await(function() return value("fade_step") == 0 end, 150, "Title fade stalled")
            wait(10)
        elseif section == 9 then
            assert(value("mouse_port") == 1, "Port 2 SNES Mouse not detected")
            capture(label)
            mouse_click("left")
            await(function() return value("screen") == 1 end, 150,
                  "Mouse click did not start gameplay")
            await(function() return value("fade_step") == 0 end, 150,
                  "Mouse-started gameplay fade stalled")
            local x = get(0)
            mouse.xOffset = 5
            await(function() return get(0) > x end, 30, "Mouse did not move paddle right")
            mouse.xOffset = -5
            await(function() return get(0) <= x end, 30, "Mouse did not move paddle left")
            mouse.xOffset = 0
            mouse_click("left")
            await(function() return get(16) == 1 end, 30, "Mouse did not launch the ball")
            mouse_click("right")
            assert(value("paused") == 1, "Mouse right click did not pause")
            local y = get(4)
            wait(12)
            assert(get(4) == y, "Mouse-paused ball moved")
            mouse_click("right")
            assert(value("paused") == 0, "Mouse right click did not resume")
            return
        elseif section == 10 then
            start()
            win()
        end
    end
    capture(label)
end

local co = coroutine.create(run)
emu.addEventCallback(function()
    if coroutine.status(co) == "dead" then return end
    local ok, message = coroutine.resume(co)
    if not ok or coroutine.status(co) == "dead" then
        local log = assert(io.open(output .. "/part" .. part .. "-section" .. section .. ".log", "w"))
        log:write(ok and "PASS\n" or "FAIL: " .. tostring(message) .. "\n")
        log:close()
        emu.stop(ok and 0 or 1)
    end
end, emu.eventType.endFrame)
emu.addEventCallback(function()
    local input = {}
    for _, key in ipairs({"a", "b", "x", "y", "l", "r", "up", "down",
                         "left", "right", "select", "start"}) do
        input[key] = buttons[key] == true
    end
    emu.setInput(input, 0, 0)
    if part == 3 and section == 9 then
        -- Mesen 2.1.1 reads the device port from setInput's third argument.
        emu.setInput(mouse, 1, 1)
    end
end, emu.eventType.inputPolled)
