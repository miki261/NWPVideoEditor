# NWPVideoEditor

A simple MFC-based video utility demonstrating:
- Importing video media into the app
- Placing a clip on a bottom timeline (double-click the clip/ drag-drop it to the timeline).
- Resize clips by dragging timeline handles.
- Optional text overlays positioned anywhere in preview window
- Export the final video with File - Save.

## Requirements
- Windows 10/11
- Visual Studio 2022 (v143 toolset)
- Windows 10 SDK (10.0.x)

## Build
Open `NWPVideoEditor.sln` in Visual Studio.

Build configurations supported:
- Debug x86, Release x86
- Debug x64, Release x64

## Run
1) File > Open and select a media file (mp4/mov/mkv/avi/mp3/wav).
2) Double-click / drag-drop the imported tile to place it on the timeline.
3) Drag the edges to change the length of the video.
4) (Optional) Click “Add Text” to add a sample overlay; multiple overlays are supported.
5) File > Save to render using ffmpeg.
6) 
## What this version does
- Displays imported video files as large icons in the main list view (MP4, MOV, MKV, AVI video formats supported).
- Visual timeline shows clips as blue bars, text overlays as orange bars.
- Real-time preview with Play/Stop buttons.
- Drag handles to trim video/text start/end points.
- Right-click clips for "Split" and "Remove" commands.
- Right-click text overlays for "Remove" command.

## License
Educational sample for coursework.
