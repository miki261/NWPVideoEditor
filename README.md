# NWPVideoEditor

A simple MFC-based video utility demonstrating:
- Importing video media into the app
- Placing a clip on a bottom timeline (double-click a tile).
- Dragging the right edge to choose output length.
- Optional text overlays (Add Text command).
- Export via ffmpeg with `-t <length>` and optional `drawtext`.

## Requirements
- Windows 10/11
- Visual Studio 2022 (v143 toolset)
- Windows 10 SDK (10.0.x)
- ffmpeg available on PATH for export

## Build
Open `NWPVideoEditor.sln` in Visual Studio.

Build configurations supported:
- Debug x86, Release x86
- Debug x64, Release x64

## Run
1) File > Import (or File > Open) and select a media file (mp4/mov/mkv/avi/mp3/wav).
2) Double-click the imported tile to place it on the timeline.
3) Drag the white right edge to pick output length.
4) (Optional) Click “Add Text” to add a sample overlay; multiple overlays are supported.
5) File > Export to render using ffmpeg.

## Notes
- If ffmpeg is not in PATH, export will fail. Add ffmpeg to PATH or update the process start command.

## Troubleshooting
- If a config fails:
  - Ensure PCH is enabled (C/C++ > Precompiled Headers) and pch.cpp uses Create (/Yc).
  - Ensure consistent Character Set = Unicode across all configs.
  - Clean Solution, then Rebuild Solution.
 
## What this version does
- Shows an empty canvas with a large-icon grid at the top and a timeline area at the bottom.
- Allows importing media files and displays them as tiles with the system icon.
- Double-clicking a tile places that clip onto the timeline for trimming.
- The right edge handle on the timeline allows setting the output length.
- “Add Text” adds a visible overlay bar scheduled near the start.
- Export creates a new mp4 using ffmpeg with the chosen length and overlay commands.

## License
Educational sample for coursework.
