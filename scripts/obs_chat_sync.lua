obs = obslua

-- ============================================================
-- AUTO CHAT BROWSER SOURCE SYNC
-- OBS Studio Lua Script
-- ============================================================

local ffi = require("ffi")

ffi.cdef[[
    typedef struct obs_service obs_service_t;
    typedef struct obs_data obs_data_t;
    obs_service_t *obs_frontend_get_streaming_service(void);
    obs_data_t *obs_service_get_settings(const obs_service_t *service);
    const char *obs_data_get_string(obs_data_t *data, const char *name);
    void obs_data_release(obs_data_t *data);
]]

local obs_frontend = nil
local obs_core = nil

target_source = "Browser"
auto_replace = true
last_detected_url = ""

local watcher_attempts = 0
local max_watcher_attempts = 20

-- ------------------------------------------------------------
-- Utilities & Logging
-- ------------------------------------------------------------

local function log_info(msg)
    obs.script_log(obs.LOG_INFO, "[Auto Chat Sync] " .. msg)
end

local function init_ffi()
    if obs_frontend ~= nil and obs_core ~= nil then
        return true
    end

    local ok1, lib1 = pcall(ffi.load, "obs-frontend-api.dll")
    local ok2, lib2 = pcall(ffi.load, "obs.dll")
    if ok1 and ok2 then
        obs_frontend = lib1
        obs_core = lib2
        return true
    end

    ok1, lib1 = pcall(ffi.load, "obs-frontend-api")
    ok2, lib2 = pcall(ffi.load, "obs")
    if ok1 and ok2 then
        obs_frontend = lib1
        obs_core = lib2
        return true
    end

    return false
end

-- ------------------------------------------------------------
-- Get YouTube Broadcast ID from OBS Streaming Service
-- ------------------------------------------------------------

local function get_broadcast_id()
    if not init_ffi() then
        log_info("Failed to initialize FFI for OBS APIs.")
        return nil
    end

    local service = obs_frontend.obs_frontend_get_streaming_service()
    if service == nil then
        return nil
    end

    local settings = obs_core.obs_service_get_settings(service)
    if settings == nil then
        return nil
    end

    local bid_ptr = obs_core.obs_data_get_string(settings, "broadcast_id")
    local bid = nil
    if bid_ptr ~= nil then
        local str = ffi.string(bid_ptr)
        if str ~= nil and str ~= "" then
            bid = str
        end
    end

    obs_core.obs_data_release(settings)
    return bid
end

-- ------------------------------------------------------------
-- Browser Source Enumeration & Helpers
-- ------------------------------------------------------------

local function get_browser_source_names()
    local names = {}
    local sources = obs.obs_enum_sources()

    if sources ~= nil then
        for _, source in ipairs(sources) do
            local source_id = obs.obs_source_get_id(source)
            if source_id == "browser_source" then
                local name = obs.obs_source_get_name(source)
                if name ~= nil and name ~= "" then
                    table.insert(names, name)
                end
            end
        end
        obs.source_list_release(sources)
    end

    table.sort(names, function(a, b)
        return string.lower(a) < string.lower(b)
    end)

    return names
end

local function add_browser_source_list(props, id, label)
    local p = obs.obs_properties_add_list(
        props,
        id,
        label,
        obs.OBS_COMBO_TYPE_LIST,
        obs.OBS_COMBO_FORMAT_STRING
    )

    obs.obs_property_list_add_string(p, "-- Select Browser Source --", "")

    local names = get_browser_source_names()
    for _, name in ipairs(names) do
        obs.obs_property_list_add_string(p, name, name)
    end

    return p
end

local function refresh_browser_source(source)
    local props = obs.obs_source_properties(source)
    if props ~= nil then
        local refresh_btn = obs.obs_properties_get(props, "refreshnocache")
        if refresh_btn ~= nil then
            obs.obs_property_button_clicked(refresh_btn, source)
            log_info("Refreshed cache of Browser Source.")
        end
        obs.obs_properties_destroy(props)
    end
end

local function apply_url_to_source(source_name, new_url)
    if source_name == nil or source_name == "" or source_name == "-- Select Browser Source --" then
        log_info("No target Browser Source selected.")
        return false
    end

    local source = obs.obs_get_source_by_name(source_name)
    if source == nil then
        log_info("Browser Source '" .. tostring(source_name) .. "' not found.")
        return false
    end

    local settings = obs.obs_source_get_settings(source)
    if settings ~= nil then
        local current_url = obs.obs_data_get_string(settings, "url")
        if current_url ~= new_url then
            obs.obs_data_set_string(settings, "url", new_url)
            obs.obs_source_update(source, settings)
            log_info("Replaced URL in '" .. source_name .. "' with: " .. new_url)
            refresh_browser_source(source)
        else
            log_info("URL in '" .. source_name .. "' is already up to date.")
        end
        obs.obs_data_release(settings)
    end

    obs.obs_source_release(source)
    return true
end

-- ------------------------------------------------------------
-- Polling Watcher on Stream Start
-- ------------------------------------------------------------

function stream_watcher_tick()
    watcher_attempts = watcher_attempts + 1

    local bid = get_broadcast_id()
    if bid ~= nil and bid ~= "" then
        obs.timer_remove(stream_watcher_tick)
        local chat_url = "https://www.youtube.com/live_chat?is_popout=1&v=" .. bid
        log_info("Detected live chat URL: " .. chat_url)
        last_detected_url = chat_url

        if auto_replace then
            apply_url_to_source(target_source, chat_url)
        end
        return
    end

    if watcher_attempts >= max_watcher_attempts then
        obs.timer_remove(stream_watcher_tick)
        log_info("Stopped searching for broadcast ID (timeout).")
    end
end

-- ------------------------------------------------------------
-- Frontend Events
-- ------------------------------------------------------------

local function on_frontend_event(event)
    if event == obs.OBS_FRONTEND_EVENT_STREAMING_STARTING or event == obs.OBS_FRONTEND_EVENT_STREAMING_STARTED then
        log_info("Live streaming started. Detecting live chat URL...")
        watcher_attempts = 0
        obs.timer_remove(stream_watcher_tick)
        obs.timer_add(stream_watcher_tick, 1000)

    elseif event == obs.OBS_FRONTEND_EVENT_STREAMING_STOPPED then
        log_info("Live streaming stopped. Resetting Browser Source to about:blank...")
        obs.timer_remove(stream_watcher_tick)
        last_detected_url = ""

        if auto_replace then
            apply_url_to_source(target_source, "about:blank")
        end
    end
end

-- ------------------------------------------------------------
-- Script Interface (Properties matching native OBS UI)
-- ------------------------------------------------------------

function refresh_button_clicked(props, prop)
    local p = obs.obs_properties_get(props, "target_source")
    if p ~= nil then
        obs.obs_property_list_clear(p)
        obs.obs_property_list_add_string(p, "-- Select Browser Source --", "")
        local names = get_browser_source_names()
        for _, name in ipairs(names) do
            obs.obs_property_list_add_string(p, name, name)
        end
    end
    return true
end

function script_properties()
    local props = obs.obs_properties_create()

    obs.obs_properties_add_text(
        props,
        "info",
        "AUTO CHAT BROWSER SOURCE SYNC\n" ..
        "When live streaming starts, the active live chat link is automatically injected\n" ..
        "into the selected Browser Source and refreshed.\n" ..
        "When live streaming stops, the Browser Source is reset to 'about:blank'.",
        obs.OBS_TEXT_INFO
    )

    add_browser_source_list(props, "target_source", "Target Browser Source")

    obs.obs_properties_add_bool(
        props,
        "auto_replace",
        "Automatically replace Browser Source URL on stream start/stop"
    )

    obs.obs_properties_add_button(
        props,
        "refresh_list",
        "⟳  Refresh Browser Source List",
        refresh_button_clicked
    )

    return props
end

function script_defaults(settings)
    obs.obs_data_set_default_string(settings, "target_source", "Browser")
    obs.obs_data_set_default_bool(settings, "auto_replace", true)
end

function script_update(settings)
    target_source = obs.obs_data_get_string(settings, "target_source")
    auto_replace = obs.obs_data_get_bool(settings, "auto_replace")
    log_info("Settings updated. Target Source: '" .. tostring(target_source) .. "', Auto-replace: " .. tostring(auto_replace))
end

function script_load(settings)
    script_update(settings)
    obs.obs_frontend_add_event_callback(on_frontend_event)
    log_info("Script loaded successfully.")
end

function script_unload()
    obs.timer_remove(stream_watcher_tick)
    obs.obs_frontend_remove_event_callback(on_frontend_event)
    log_info("Script unloaded.")
end

function script_description()
    return [[
Auto Chat Browser Source Sync

Automatically syncs the YouTube Live Chat popout URL to your chosen Browser Source whenever live streaming starts.
When live streaming stops, it automatically resets the Browser Source to about:blank so the overlay stays clean.

UI automatically adapts to your OBS Studio theme (Dark/Light).
]]
end
