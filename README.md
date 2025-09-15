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
1) Download FFmpeg from https://ffmpeg.org/ (You need ffmpeg.exe and ffprobe.exe from the bin folder).
2) Settings > Load FFmpeg and locate your ffmpeg folder.
3) File > Open and select a media file (mp4/mov/mkv/avi/mp3/wav).
4) Double-click / drag-drop the imported tile to place it on the timeline.
5) Drag the edges to change the length of the video.
6) (Optional) Click “Add Text” to add a sample overlay; multiple overlays are supported.
7) File > Save to render using ffmpeg.
   
## What this version does
- Displays imported video files as large icons in the main list view (MP4, MOV, MKV, AVI video formats supported).
- Visual timeline shows clips as blue bars, text overlays as orange bars.
- Real-time preview with Play/Stop buttons.
- Drag handles to trim video/text start/end points.
- Right-click clips for "Split" and "Remove" commands.
- Right-click text overlays for "Remove" command.

## License
Educational sample for coursework.
