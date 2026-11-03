# OBS Auto Chat Browser Source Sync

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Windows%20(x64)-brightgreen.svg)](https://github.com/darkero27/obs-chat-sync/releases/tag/v1.0.0)
[![OBS Studio](https://img.shields.io/badge/OBS%20Studio-30%20|%2031%20|%2032+-orange.svg)](https://obsproject.com)

OBS Auto Chat Sync is an open-source tool and plugin for OBS Studio that automatically injects your live stream chat link (such as YouTube Live Chat) into a designated Browser Source overlay every time you start streaming, and cleanly resets it to about:blank when streaming ends.

---

## The Problem It Solves

When streaming on YouTube and using an on-screen live chat or CSS livechat overlay, you usually have to manually copy the new live chat popout link and paste it into your OBS Browser Source before every stream. With OBS Auto Chat Sync, you never have to manually copy-paste live chat URLs again!

---

## Available Versions

This project comes in two formats, giving you full flexibility:

| Feature | Built-in Plugin DLL | Native Lua Script |
| :--- | :--- | :--- |
| Location | Tools -> Auto Chat Browser Source Settings | Tools -> Scripts |
| Installation | Run OBS-Chat-Sync-Installer.exe (1-click install) | Add script in OBS Script Manager |
| Theme Adaptability | Dynamically adapts to OBS Theme (Dark / Light) | Rendered directly by OBS Qt Property engine |
| Auto-Replace on Start | Yes | Yes |
| Auto-Refresh Cache | Yes | Yes |
| Auto-Reset to about:blank | Yes | Yes |
| Zero Dependencies | Yes (Pure C and Win32/OBS API) | Yes (Pure Lua + LuaJIT FFI) |

---

## Quick Start and Installation

### Method 1: Installer EXE (Recommended)
1. Download OBS-Chat-Sync-Installer.exe from the GitHub Releases page (https://github.com/darkero27/obs-chat-sync/releases).
2. Close OBS Studio if it is currently running.
3. Run OBS-Chat-Sync-Installer.exe.
4. The installer automatically detects your OBS Studio installation (Steam or Standard 64-bit) and installs the plugin DLL directly into OBS.
5. Launch OBS Studio. You will now see Auto Chat Browser Source Settings under the Tools menu.

---

### Method 2: Lua Script (obs_chat_sync.lua)
1. Download obs_chat_sync.lua from the GitHub Releases page (or from the scripts folder).
2. Open OBS Studio.
3. Go to top menu: Tools -> Scripts.
4. Click the + (Add) button at the bottom left.
5. Select obs_chat_sync.lua.
6. The script settings will appear in the right panel, matching standard OBS script styling.

---

## Usage Guide and Demo Walkthrough

### Step 1: Initial Setup (Do this once)
1. In OBS Studio, create a Browser Source in your scene where you want your live chat overlay to display (for example, named Browser or Chat Overlay).
2. Open the settings:
   - For DLL version: Click Tools -> Auto Chat Browser Source Settings.
   - For Lua Script version: Go to Tools -> Scripts and click obs_chat_sync.lua.
3. Select your target Browser Source from the dropdown list:
   Target Browser Source: [ Browser ]
4. Ensure the checkbox is checked:
   [x] Automatically replace Browser Source URL on stream start/stop
5. Click Save (or close the Scripts window).

---

### Step 2: Live Stream Demo (What happens automatically)

```
[ STREAM START ]
       |
       v
1. You click "Start Streaming" in OBS
       |
       v
2. OBS initializes the YouTube broadcast and opens the side Chat Dock
       |
       v
3. OBS Auto Chat Sync detects the new Broadcast ID (such as qxO1um7Vxqk)
       |
       v
4. Formulates popout URL:
   https://www.youtube.com/live_chat?is_popout=1&v=qxO1um7Vxqk
       |
       v
5. Automatically replaces URL in your chosen Browser Source
       |
       v
6. Automatically triggers "Refresh cache of current page"
   Result: Live Chat immediately displays on your stream overlay!
```

---

## How It Works (Core Concept)

Under the hood, OBS Auto Chat Sync bridges the gap between OBS Studio's internal streaming session and your scene's Browser Source:

```
+-------------------------+
|     OBS Studio Core     |
|   (Streaming Service)   |
+------------+------------+
             | 1. Emits OBS_FRONTEND_EVENT_STREAMING_STARTED
             v
+-------------------------+
|   OBS Auto Chat Sync    | 2. Queries obs_service_get_settings()
|   (DLL / Lua Script)    |    Extracts "broadcast_id" (such as qxO1um7Vxqk)
+------------+------------+
             | 3. Formulates Popout URL:
             |    https://www.youtube.com/live_chat?is_popout=1&v={ID}
             v
+-------------------------+
|   Target Browser Source | 4. Updates "url" setting via obs_source_update()
|      (Chat Overlay)     | 5. Triggers "refreshnocache" property button
+-------------------------+
```

### 1. Event Hooking via OBS Frontend API
The tool registers an event listener with OBS Studio (obs_frontend_add_event_callback). When you click Start Streaming, OBS emits the event OBS_FRONTEND_EVENT_STREAMING_STARTED.

### 2. Extracting Active YouTube Broadcast ID
When you broadcast to YouTube through OBS Studio's built-in integration, OBS assigns a unique session ID for the live stream. The tool accesses the active streaming service via obs_frontend_get_streaming_service(), reads its settings data (obs_service_get_settings), and extracts the active broadcast_id (the YouTube Video ID for that specific stream).

### 3. Dynamic Popout URL Construction
Using the extracted broadcast ID, the tool generates the official YouTube Live Chat popout URL:
```text
https://www.youtube.com/live_chat?is_popout=1&v={BROADCAST_ID}
```

### 4. Updating the Browser Source and Cache Refresh
- The tool searches for your selected Browser Source by name (obs_get_source_by_name).
- It applies the new chat URL into the source's data settings (obs_data_set_string(settings, "url", ...)).
- It programmatically triggers the source's internal refreshnocache property button. This performs the exact same action as clicking "Refresh cache of current page" in OBS, ensuring your custom CSS and chat styling reloads cleanly with the new chat room.

### 5. Automatic Cleanup on Stream End
When you click Stop Streaming, OBS emits OBS_FRONTEND_EVENT_STREAMING_STOPPED. The tool intercepts this event and resets the Browser Source URL to about:blank, preventing old, frozen chat overlays from remaining on your scene.

### 6. Dual-Architecture Implementation
- Built-in DLL (Pure C): Directly interacts with the native OBS C API and OBS Frontend API, providing optimal performance and a native Win32 settings dialog that automatically matches OBS Dark/Light themes (obs_frontend_is_theme_dark()).
- Lua Script (LuaJIT FFI): Uses LuaJIT's Foreign Function Interface (ffi) to dynamically link against OBS Studio's loaded process memory (obs-frontend-api.dll and obs.dll), granting the script full C-level API capabilities from within OBS Studio's standard Script Manager.

---

## Building from Source

To compile obs-chat-sync.dll and OBS-Chat-Sync-Installer.exe yourself:

### Requirements:
- Windows 10 / 11 (64-bit)
- Visual Studio 2022 with C++ Build Tools installed
- Windows SDK

### Build Steps:
1. Open a command prompt or PowerShell.
2. Navigate to the src folder:
   ```cmd
   cd src
   ```
3. Run build.bat:
   ```cmd
   build.bat
   ```
4. The compiled binaries will be output to bin\64bit\obs-chat-sync.dll and release\OBS-Chat-Sync-Installer.exe.

---

## Repository Structure

```
obs-chat-sync/
|-- bin/
|   \-- 64bit/
|       \-- obs-chat-sync.dll
|-- scripts/
|   \-- obs_chat_sync.lua
|-- src/
|   |-- obs_chat_sync.c
|   |-- installer.c
|   |-- installer.rc
|   |-- installer.manifest
|   |-- build.bat
|   \-- lib/
|-- LICENSE
\-- README.md
```

---

## License

This project is licensed under the MIT License - see the LICENSE (LICENSE) file for details.
