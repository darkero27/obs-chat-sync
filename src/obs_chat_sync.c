#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

#define EXPORT __declspec(dllexport)
#define IMPORT __declspec(dllimport)

/* DWM dark mode attribute definitions */
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif
#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif
#ifndef DWMWA_TEXT_COLOR
#define DWMWA_TEXT_COLOR 36
#endif

/* OBS Types */
typedef struct obs_module obs_module_t;
typedef struct obs_source obs_source_t;
typedef struct obs_data obs_data_t;
typedef struct obs_properties obs_properties_t;
typedef struct obs_property obs_property_t;
typedef struct obs_service obs_service_t;

enum obs_frontend_event {
    OBS_FRONTEND_EVENT_STREAMING_STARTING,
    OBS_FRONTEND_EVENT_STREAMING_STARTED,
    OBS_FRONTEND_EVENT_STREAMING_STOPPING,
    OBS_FRONTEND_EVENT_STREAMING_STOPPED,
    OBS_FRONTEND_EVENT_RECORDING_STARTING,
    OBS_FRONTEND_EVENT_RECORDING_STARTED,
    OBS_FRONTEND_EVENT_RECORDING_STOPPING,
    OBS_FRONTEND_EVENT_RECORDING_STOPPED,
    OBS_FRONTEND_EVENT_SCENE_CHANGED,
    OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED,
    OBS_FRONTEND_EVENT_TRANSITION_CHANGED,
    OBS_FRONTEND_EVENT_TRANSITION_STOPPED,
    OBS_FRONTEND_EVENT_TRANSITION_DURATION_CHANGED,
    OBS_FRONTEND_EVENT_TBAR_CHANGED,
    OBS_FRONTEND_EVENT_PROFILE_CHANGED,
    OBS_FRONTEND_EVENT_PROFILE_LIST_CHANGED,
    OBS_FRONTEND_EVENT_EXIT,
};

typedef void (*obs_frontend_cb)(void *private_data);
typedef void (*obs_frontend_event_cb)(enum obs_frontend_event event, void *private_data);

/* Frontend API */
IMPORT void obs_frontend_add_tools_menu_item(const char *name, obs_frontend_cb callback, void *private_data);
IMPORT void obs_frontend_add_event_callback(obs_frontend_event_cb callback, void *private_data);
IMPORT void obs_frontend_remove_event_callback(obs_frontend_event_cb callback, void *private_data);
IMPORT void *obs_frontend_get_main_window_handle(void);
IMPORT obs_service_t *obs_frontend_get_streaming_service(void);
IMPORT bool obs_frontend_is_theme_dark(void);

/* libobs API */
IMPORT void obs_enum_sources(bool (*enum_proc)(void *param, obs_source_t *source), void *param);
IMPORT obs_source_t *obs_get_source_by_name(const char *name);
IMPORT obs_data_t *obs_source_get_settings(const obs_source_t *source);
IMPORT void obs_source_update(obs_source_t *source, obs_data_t *settings);
IMPORT void obs_source_release(obs_source_t *source);
IMPORT const char *obs_source_get_name(const obs_source_t *source);
IMPORT const char *obs_source_get_id(const obs_source_t *source);

IMPORT obs_data_t *obs_service_get_settings(const obs_service_t *service);

IMPORT const char *obs_data_get_string(obs_data_t *data, const char *name);
IMPORT void obs_data_set_string(obs_data_t *data, const char *name, const char *val);
IMPORT bool obs_data_get_bool(obs_data_t *data, const char *name);
IMPORT void obs_data_set_bool(obs_data_t *data, const char *name, bool val);
IMPORT void obs_data_release(obs_data_t *data);
IMPORT obs_data_t *obs_data_create(void);
IMPORT obs_data_t *obs_data_create_from_json_file(const char *file);
IMPORT bool obs_data_save_json_safe(obs_data_t *data, const char *file, const char *temp_ext, const char *backup_ext);

IMPORT obs_properties_t *obs_source_properties(const obs_source_t *source);
IMPORT obs_property_t *obs_properties_get(obs_properties_t *props, const char *property);
IMPORT bool obs_property_button_clicked(obs_property_t *prop, obs_source_t *source);
IMPORT void obs_properties_destroy(obs_properties_t *props);

IMPORT void blog(int log_level, const char *format, ...);

#define LOG_INFO 200
#define LOG_WARNING 300

/* Theme Color definitions dynamically adapting to OBS user theme */
typedef struct {
    COLORREF bg;
    COLORREF control_bg;
    COLORREF text;
    COLORREF muted_text;
    COLORREF border;
    COLORREF btn_bg;
    COLORREF btn_hover;
    COLORREF btn_pressed;
    COLORREF accent;
    COLORREF accent_hover;
    COLORREF accent_pressed;
    BOOL dark_title_bar;
} ThemeColors;

static ThemeColors theme_dark = {
    RGB(30, 32, 38),     /* bg */
    RGB(40, 44, 52),     /* control_bg */
    RGB(230, 234, 240),  /* text */
    RGB(140, 148, 160),  /* muted_text */
    RGB(62, 68, 80),     /* border */
    RGB(48, 53, 64),     /* btn_bg */
    RGB(60, 67, 80),     /* btn_hover */
    RGB(40, 45, 55),     /* btn_pressed */
    RGB(52, 84, 130),    /* accent */
    RGB(64, 104, 160),   /* accent_hover */
    RGB(42, 70, 110),    /* accent_pressed */
    TRUE                 /* dark_title_bar */
};

static ThemeColors theme_light = {
    RGB(242, 243, 245),  /* bg */
    RGB(255, 255, 255),  /* control_bg */
    RGB(24, 26, 30),     /* text */
    RGB(110, 118, 130),  /* muted_text */
    RGB(200, 205, 215),  /* border */
    RGB(230, 233, 238),  /* btn_bg */
    RGB(220, 224, 230),  /* btn_hover */
    RGB(205, 210, 218),  /* btn_pressed */
    RGB(46, 110, 210),   /* accent */
    RGB(36, 95, 190),    /* accent_hover */
    RGB(26, 80, 170),    /* accent_pressed */
    FALSE                /* dark_title_bar */
};

static ThemeColors curTheme;

static HBRUSH hBrushBg = NULL;
static HBRUSH hBrushControl = NULL;
static HBRUSH hBrushBtn = NULL;
static HBRUSH hBrushBtnHover = NULL;
static HBRUSH hBrushBtnPressed = NULL;
static HBRUSH hBrushAccent = NULL;
static HBRUSH hBrushAccentHover = NULL;
static HBRUSH hBrushAccentPressed = NULL;
static HPEN   hPenBorder = NULL;
static HFONT  hFontSegoe = NULL;
static HFONT  hFontSegoeBold = NULL;

/* Plugin configuration */
static struct {
    char target_source[256];
    bool auto_replace;
    char last_detected_url[512];
    char last_broadcast_id[128];
} config = {
    "Browser",
    true,
    "",
    ""
};

static obs_module_t *module_ptr = NULL;
static HANDLE watcher_thread = NULL;
static volatile bool watcher_running = false;

static void get_config_path(char *out_path, size_t max_len) {
    char appdata[MAX_PATH];
    if (GetEnvironmentVariableA("APPDATA", appdata, MAX_PATH) > 0) {
        snprintf(out_path, max_len, "%s\\obs-studio\\plugin_config\\obs-chat-sync", appdata);
        CreateDirectoryA(out_path, NULL);
        snprintf(out_path, max_len, "%s\\obs-studio\\plugin_config\\obs-chat-sync\\config.json", appdata);
    } else {
        snprintf(out_path, max_len, "obs-chat-sync.json");
    }
}

static void save_config(void) {
    char path[MAX_PATH];
    get_config_path(path, sizeof(path));

    obs_data_t *data = obs_data_create();
    obs_data_set_string(data, "target_source", config.target_source);
    obs_data_set_bool(data, "auto_replace", config.auto_replace);
    obs_data_set_string(data, "last_detected_url", config.last_detected_url);
    obs_data_set_string(data, "last_broadcast_id", config.last_broadcast_id);

    obs_data_save_json_safe(data, path, ".tmp", ".bak");
    obs_data_release(data);
    blog(LOG_INFO, "[AutoChatSync] Configuration saved to: %s", path);
}

static void load_config(void) {
    char path[MAX_PATH];
    get_config_path(path, sizeof(path));

    obs_data_t *data = obs_data_create_from_json_file(path);
    if (data) {
        const char *src = obs_data_get_string(data, "target_source");
        if (src && *src) {
            strncpy(config.target_source, src, sizeof(config.target_source) - 1);
        }
        config.auto_replace = obs_data_get_bool(data, "auto_replace");

        const char *last_url = obs_data_get_string(data, "last_detected_url");
        if (last_url && *last_url) {
            strncpy(config.last_detected_url, last_url, sizeof(config.last_detected_url) - 1);
        }
        const char *last_bid = obs_data_get_string(data, "last_broadcast_id");
        if (last_bid && *last_bid) {
            strncpy(config.last_broadcast_id, last_bid, sizeof(config.last_broadcast_id) - 1);
        }

        obs_data_release(data);
        blog(LOG_INFO, "[AutoChatSync] Loaded config. Target Source: '%s', AutoReplace: %d",
             config.target_source, config.auto_replace);
    } else {
        blog(LOG_INFO, "[AutoChatSync] Using default config. Target Source: '%s'", config.target_source);
    }
}

/* Detect Live Chat URL without forced YouTube dark theme */
static bool detect_live_chat_url(char *out_url, size_t max_len) {
    obs_service_t *service = obs_frontend_get_streaming_service();
    if (service) {
        obs_data_t *settings = obs_service_get_settings(service);
        if (settings) {
            const char *bid = obs_data_get_string(settings, "broadcast_id");
            if (bid && *bid) {
                /* Standard popout live chat URL without forced dark theme */
                snprintf(out_url, max_len, "https://www.youtube.com/live_chat?is_popout=1&v=%s", bid);
                strncpy(config.last_broadcast_id, bid, sizeof(config.last_broadcast_id) - 1);
                strncpy(config.last_detected_url, out_url, sizeof(config.last_detected_url) - 1);
                obs_data_release(settings);
                return true;
            }
            obs_data_release(settings);
        }
    }

    if (config.last_detected_url[0] != '\0') {
        strncpy(out_url, config.last_detected_url, max_len - 1);
        return true;
    }

    return false;
}

/* Apply new URL to target Browser Source and refresh cache automatically */
static bool apply_chat_url_to_source(const char *chat_url) {
    if (!chat_url || *chat_url == '\0') {
        blog(LOG_WARNING, "[AutoChatSync] Cannot apply empty URL.");
        return false;
    }

    if (config.target_source[0] == '\0') {
        blog(LOG_WARNING, "[AutoChatSync] No target Browser Source configured.");
        return false;
    }

    obs_source_t *source = obs_get_source_by_name(config.target_source);
    if (!source) {
        blog(LOG_WARNING, "[AutoChatSync] Browser Source '%s' not found!", config.target_source);
        return false;
    }

    obs_data_t *settings = obs_source_get_settings(source);
    if (!settings) {
        obs_source_release(source);
        return false;
    }

    const char *curr_url = obs_data_get_string(settings, "url");
    bool changed = (curr_url == NULL || strcmp(curr_url, chat_url) != 0);

    if (changed) {
        obs_data_set_string(settings, "url", chat_url);
        obs_source_update(source, settings);
        blog(LOG_INFO, "[AutoChatSync] Updated Browser Source '%s' URL to: %s", config.target_source, chat_url);

        /* Refresh cache automatically */
        obs_properties_t *props = obs_source_properties(source);
        if (props) {
            obs_property_t *refresh_btn = obs_properties_get(props, "refreshnocache");
            if (refresh_btn) {
                obs_property_button_clicked(refresh_btn, source);
                blog(LOG_INFO, "[AutoChatSync] Refreshed Browser Source cache.");
            }
            obs_properties_destroy(props);
        }
    } else {
        blog(LOG_INFO, "[AutoChatSync] URL in '%s' is already up to date.", config.target_source);
    }

    obs_data_release(settings);
    obs_source_release(source);
    return true;
}

/* Background thread watcher for stream startup */
static DWORD WINAPI stream_watcher_proc(LPVOID lpParam) {
    (void)lpParam;
    blog(LOG_INFO, "[AutoChatSync] Stream watcher started, polling for broadcast_id...");

    char detected_url[512] = {0};
    int attempts = 0;
    const int max_attempts = 20;

    while (watcher_running && attempts < max_attempts) {
        Sleep(1000);
        attempts++;

        if (detect_live_chat_url(detected_url, sizeof(detected_url))) {
            blog(LOG_INFO, "[AutoChatSync] Live chat URL detected on attempt %d: %s", attempts, detected_url);
            if (config.auto_replace) {
                apply_chat_url_to_source(detected_url);
            }
            break;
        }
    }

    watcher_running = false;
    blog(LOG_INFO, "[AutoChatSync] Stream watcher ended.");
    return 0;
}

static void start_stream_watcher(void) {
    char url[512];
    if (detect_live_chat_url(url, sizeof(url))) {
        blog(LOG_INFO, "[AutoChatSync] Live chat URL available immediately: %s", url);
        if (config.auto_replace) {
            apply_chat_url_to_source(url);
        }
        return;
    }

    if (!watcher_running) {
        watcher_running = true;
        if (watcher_thread) {
            CloseHandle(watcher_thread);
        }
        watcher_thread = CreateThread(NULL, 0, stream_watcher_proc, NULL, 0, NULL);
    }
}

/* Frontend event callback */
static void on_frontend_event(enum obs_frontend_event event, void *param) {
    (void)param;
    if (event == OBS_FRONTEND_EVENT_STREAMING_STARTING || event == OBS_FRONTEND_EVENT_STREAMING_STARTED) {
        blog(LOG_INFO, "[AutoChatSync] Streaming started, initiating live chat URL detection...");
        start_stream_watcher();
    } else if (event == OBS_FRONTEND_EVENT_STREAMING_STOPPED) {
        blog(LOG_INFO, "[AutoChatSync] Streaming stopped. Resetting Browser Source to about:blank...");
        watcher_running = false;
        config.last_detected_url[0] = '\0';
        config.last_broadcast_id[0] = '\0';
        if (config.auto_replace) {
            apply_chat_url_to_source("about:blank");
        }
    }
}

/* ========================================================================= */
/* DYNAMIC THEMED SETTINGS GUI DIALOG (Follows OBS User Theme Light/Dark)    */
/* ========================================================================= */

#define ID_COMBO_SOURCES 1001
#define ID_EDIT_URL      1002
#define ID_CHK_REPLACE   1003
#define ID_BTN_SAVE      1004
#define ID_BTN_CANCEL    1005

static HWND hDlgWnd = NULL;
static HWND hComboSources = NULL;
static HWND hEditUrl = NULL;
static HWND hChkReplace = NULL;
static HWND hBtnSave = NULL;
static HWND hBtnCancel = NULL;

static bool enum_populate_sources(void *param, obs_source_t *source) {
    HWND hCombo = (HWND)param;
    const char *id = obs_source_get_id(source);
    if (id && strcmp(id, "browser_source") == 0) {
        const char *name = obs_source_get_name(source);
        if (name && *name) {
            SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)name);
        }
    }
    return true;
}

static void draw_custom_button(LPDRAWITEMSTRUCT dis, bool isAccent, const wchar_t *text) {
    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;
    bool isPressed = (dis->itemState & ODS_SELECTED) != 0;

    HBRUSH bgBrush;
    if (isAccent) {
        bgBrush = isPressed ? hBrushAccentPressed : hBrushAccent;
    } else {
        bgBrush = isPressed ? hBrushBtnPressed : hBrushBtn;
    }

    HPEN oldPen = (HPEN)SelectObject(hdc, hPenBorder);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, bgBrush);

    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 6, 6);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, isAccent ? RGB(255, 255, 255) : curTheme.text);
    HFONT oldFont = (HFONT)SelectObject(hdc, isAccent ? hFontSegoeBold : hFontSegoe);
    
    if (isPressed) {
        OffsetRect(&rc, 1, 1);
    }
    DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
}

static LRESULT CALLBACK SettingsDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        /* Detect OBS theme dynamically */
        bool isDark = obs_frontend_is_theme_dark();
        curTheme = isDark ? theme_dark : theme_light;

        /* Set DWM title bar according to OBS theme */
        BOOL darkMode = curTheme.dark_title_bar;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
        DwmSetWindowAttribute(hwnd, 19, &darkMode, sizeof(darkMode));
        COLORREF captionCol = curTheme.bg;
        DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &captionCol, sizeof(captionCol));

        /* Create GDI brushes & pens for the current theme */
        hBrushBg            = CreateSolidBrush(curTheme.bg);
        hBrushControl       = CreateSolidBrush(curTheme.control_bg);
        hBrushBtn           = CreateSolidBrush(curTheme.btn_bg);
        hBrushBtnHover      = CreateSolidBrush(curTheme.btn_hover);
        hBrushBtnPressed    = CreateSolidBrush(curTheme.btn_pressed);
        hBrushAccent        = CreateSolidBrush(curTheme.accent);
        hBrushAccentHover   = CreateSolidBrush(curTheme.accent_hover);
        hBrushAccentPressed = CreateSolidBrush(curTheme.accent_pressed);
        hPenBorder          = CreatePen(PS_SOLID, 1, curTheme.border);

        hFontSegoe     = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                    VARIABLE_PITCH | FF_SWISS, L"Segoe UI");
        hFontSegoeBold = CreateFontW(15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                    VARIABLE_PITCH | FF_SWISS, L"Segoe UI");

        /* Label 1: Target Browser Source */
        HWND lbl1 = CreateWindowW(L"STATIC", L"Target Browser Source:",
                                  WS_CHILD | WS_VISIBLE, 24, 20, 420, 20, hwnd, NULL, NULL, NULL);
        SendMessageW(lbl1, WM_SETFONT, (WPARAM)hFontSegoeBold, TRUE);

        /* ComboBox for Browser Sources */
        hComboSources = CreateWindowW(L"COMBOBOX", L"",
                                      WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                                      24, 45, 432, 220, hwnd, (HMENU)ID_COMBO_SOURCES, NULL, NULL);
        SendMessageW(hComboSources, WM_SETFONT, (WPARAM)hFontSegoe, TRUE);

        /* Populate with browser sources from OBS */
        obs_enum_sources(enum_populate_sources, (void*)hComboSources);

        /* Select configured source */
        int sel_idx = (int)SendMessageA(hComboSources, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)config.target_source);
        if (sel_idx != CB_ERR) {
            SendMessageW(hComboSources, CB_SETCURSEL, sel_idx, 0);
        } else {
            SendMessageW(hComboSources, CB_SETCURSEL, 0, 0);
        }

        /* Label 2: Detected Live Chat URL */
        HWND lbl2 = CreateWindowW(L"STATIC", L"Active Live Chat URL:",
                                  WS_CHILD | WS_VISIBLE, 24, 85, 420, 20, hwnd, NULL, NULL, NULL);
        SendMessageW(lbl2, WM_SETFONT, (WPARAM)hFontSegoeBold, TRUE);

        /* Edit box for detected URL */
        char current_url[512] = {0};
        detect_live_chat_url(current_url, sizeof(current_url));
        if (current_url[0] == '\0') {
            snprintf(current_url, sizeof(current_url), "Waiting for live stream to start...");
        }

        hEditUrl = CreateWindowW(L"EDIT", L"",
                                 WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY | WS_TABSTOP,
                                 24, 110, 432, 25, hwnd, (HMENU)ID_EDIT_URL, NULL, NULL);
        SendMessageW(hEditUrl, WM_SETFONT, (WPARAM)hFontSegoe, TRUE);
        SetWindowTextA(hEditUrl, current_url);

        /* Checkbox: Auto-replace */
        hChkReplace = CreateWindowW(L"BUTTON", L"Automatically replace Browser Source URL on stream start/stop",
                                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                                    24, 150, 432, 22, hwnd, (HMENU)ID_CHK_REPLACE, NULL, NULL);
        SendMessageW(hChkReplace, WM_SETFONT, (WPARAM)hFontSegoe, TRUE);
        SendMessageW(hChkReplace, BM_SETCHECK, config.auto_replace ? BST_CHECKED : BST_UNCHECKED, 0);

        /* Buttons with owner-draw styling matching current theme */
        hBtnSave = CreateWindowW(L"BUTTON", L"Save",
                                 WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_TABSTOP,
                                 260, 190, 95, 32, hwnd, (HMENU)ID_BTN_SAVE, NULL, NULL);

        hBtnCancel = CreateWindowW(L"BUTTON", L"Cancel",
                                   WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_TABSTOP,
                                   365, 190, 91, 32, hwnd, (HMENU)ID_BTN_CANCEL, NULL, NULL);

        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, curTheme.text);
        return (INT_PTR)hBrushBg;
    }

    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, curTheme.control_bg);
        SetTextColor(hdc, curTheme.text);
        return (INT_PTR)hBrushControl;
    }

    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, curTheme.control_bg);
        SetTextColor(hdc, curTheme.text);
        return (INT_PTR)hBrushControl;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        if (dis->CtlID == ID_BTN_SAVE) {
            draw_custom_button(dis, true, L"Save");
            return TRUE;
        } else if (dis->CtlID == ID_BTN_CANCEL) {
            draw_custom_button(dis, false, L"Cancel");
            return TRUE;
        }
        break;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == ID_BTN_SAVE) {
            char selected_source[256] = {0};
            int cur = (int)SendMessageW(hComboSources, CB_GETCURSEL, 0, 0);
            if (cur != CB_ERR) {
                SendMessageA(hComboSources, CB_GETLBTEXT, cur, (LPARAM)selected_source);
                strncpy(config.target_source, selected_source, sizeof(config.target_source) - 1);
            }

            config.auto_replace = (SendMessageW(hChkReplace, BM_GETCHECK, 0, 0) == BST_CHECKED);

            save_config();
            DestroyWindow(hwnd);
        } else if (id == ID_BTN_CANCEL) {
            DestroyWindow(hwnd);
        }
        return 0;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY: {
        HWND obsHwnd = (HWND)obs_frontend_get_main_window_handle();
        if (obsHwnd) {
            EnableWindow(obsHwnd, TRUE);
            SetForegroundWindow(obsHwnd);
        }

        if (hFontSegoe)          { DeleteObject(hFontSegoe); hFontSegoe = NULL; }
        if (hFontSegoeBold)      { DeleteObject(hFontSegoeBold); hFontSegoeBold = NULL; }
        if (hBrushBg)            { DeleteObject(hBrushBg); hBrushBg = NULL; }
        if (hBrushControl)       { DeleteObject(hBrushControl); hBrushControl = NULL; }
        if (hBrushBtn)           { DeleteObject(hBrushBtn); hBrushBtn = NULL; }
        if (hBrushBtnHover)      { DeleteObject(hBrushBtnHover); hBrushBtnHover = NULL; }
        if (hBrushBtnPressed)    { DeleteObject(hBrushBtnPressed); hBrushBtnPressed = NULL; }
        if (hBrushAccent)        { DeleteObject(hBrushAccent); hBrushAccent = NULL; }
        if (hBrushAccentHover)   { DeleteObject(hBrushAccentHover); hBrushAccentHover = NULL; }
        if (hBrushAccentPressed) { DeleteObject(hBrushAccentPressed); hBrushAccentPressed = NULL; }
        if (hPenBorder)          { DeleteObject(hPenBorder); hPenBorder = NULL; }

        hDlgWnd = NULL;
        return 0;
    }

    default:
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

static void on_menu_clicked(void *param) {
    (void)param;
    if (hDlgWnd && IsWindow(hDlgWnd)) {
        SetForegroundWindow(hDlgWnd);
        return;
    }

    HINSTANCE hInst = GetModuleHandleW(NULL);
    HWND obsHwnd = (HWND)obs_frontend_get_main_window_handle();

    bool isDark = obs_frontend_is_theme_dark();
    ThemeColors theme = isDark ? theme_dark : theme_light;

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = SettingsDlgProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"OBSAutoChatSyncDynamicThemeDlg";
    wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(theme.bg);

    RegisterClassExW(&wc);

    RECT rcObs;
    int width = 490, height = 280;
    int x = CW_USEDEFAULT, y = CW_USEDEFAULT;
    if (obsHwnd && GetWindowRect(obsHwnd, &rcObs)) {
        x = rcObs.left + (rcObs.right - rcObs.left - width) / 2;
        y = rcObs.top + (rcObs.bottom - rcObs.top - height) / 2;
    }

    hDlgWnd = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
                              L"OBSAutoChatSyncDynamicThemeDlg",
                              L"Auto Chat Browser Source Settings - OBS Studio",
                              WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                              x, y, width, height,
                              obsHwnd, NULL, hInst, NULL);

    if (obsHwnd) {
        EnableWindow(obsHwnd, FALSE);
    }

    ShowWindow(hDlgWnd, SW_SHOW);
    UpdateWindow(hDlgWnd);
}

/* ========================================================================= */
/* MODULE EXPORTS                                                            */
/* ========================================================================= */

EXPORT uint32_t obs_module_ver(void) {
    return 0x20020002; /* OBS 32.2.2 compatible */
}

EXPORT void obs_module_set_pointer(obs_module_t *module) {
    module_ptr = module;
}

EXPORT const char *obs_module_name(void) {
    return "Auto Chat Browser Source Sync";
}

EXPORT const char *obs_module_description(void) {
    return "Automatically syncs live stream chat dock URL to a chosen Browser Source.";
}

EXPORT void obs_module_set_locale(const char *locale) {
    (void)locale;
}

EXPORT void obs_module_free_locale(void) {
}

EXPORT bool obs_module_load(void) {
    blog(LOG_INFO, "[AutoChatSync] Loading built-in plugin v1.3.0...");

    load_config();

    obs_frontend_add_tools_menu_item("Auto Chat Browser Source Settings", on_menu_clicked, NULL);
    obs_frontend_add_event_callback(on_frontend_event, NULL);

    char url[512] = {0};
    if (detect_live_chat_url(url, sizeof(url))) {
        blog(LOG_INFO, "[AutoChatSync] Initial live chat URL detected: %s", url);
    }

    blog(LOG_INFO, "[AutoChatSync] Built-in plugin loaded successfully!");
    return true;
}

EXPORT void obs_module_unload(void) {
    watcher_running = false;
    if (watcher_thread) {
        WaitForSingleObject(watcher_thread, 1000);
        CloseHandle(watcher_thread);
        watcher_thread = NULL;
    }

    obs_frontend_remove_event_callback(on_frontend_event, NULL);

    if (hDlgWnd && IsWindow(hDlgWnd)) {
        DestroyWindow(hDlgWnd);
    }

    blog(LOG_INFO, "[AutoChatSync] Plugin unloaded.");
}
