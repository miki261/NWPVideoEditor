#include "pch.h"
#include "framework.h"
#include "NWPVideoEditor.h"
#include "NWPVideoEditorDoc.h"
#include "NWPVideoEditorView.h"
#include "Resource.h"
#include <shlobj.h>
#include <shellapi.h>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(NWPVideoEditorView, CView)

BEGIN_MESSAGE_MAP(NWPVideoEditorView, CView)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_COMMAND(ID_FILE_IMPORT, &NWPVideoEditorView::OnFileImport)
    ON_COMMAND(ID_FILE_OPEN, &NWPVideoEditorView::OnFileImport)
    ON_COMMAND(ID_FILE_EXPORT, &NWPVideoEditorView::OnFileExport)
    ON_COMMAND(ID_FILE_SAVE, &NWPVideoEditorView::OnFileSave)
    ON_COMMAND(ID_EDIT_ADDTEXT, &NWPVideoEditorView::OnEditAddText)
    ON_BN_CLICKED(ID_ADD_TEXT_BUTTON, &NWPVideoEditorView::OnAddText)
    ON_NOTIFY(NM_DBLCLK, 1001, &NWPVideoEditorView::OnListDblClk)
    ON_NOTIFY(LVN_ITEMACTIVATE, 1001, &NWPVideoEditorView::OnListItemActivate)
    ON_NOTIFY(LVN_BEGINDRAG, 1001, &NWPVideoEditorView::OnListBeginDrag)
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
    ON_WM_RBUTTONDOWN()
    ON_WM_CONTEXTMENU()
    ON_COMMAND(ID_TIMELINE_REMOVE_CLIP, &NWPVideoEditorView::OnTimelineRemoveClip)
    ON_COMMAND(ID_TIMELINE_SPLIT_CLIP, &NWPVideoEditorView::OnTimelineSplitClip)
    ON_COMMAND(32775, &NWPVideoEditorView::OnDeleteTextOverlay)
    ON_BN_CLICKED(ID_PLAY_PAUSE_BUTTON, &NWPVideoEditorView::OnPlayPause)
    ON_BN_CLICKED(ID_STOP_BUTTON, &NWPVideoEditorView::OnStop)
    ON_COMMAND(ID_SETTINGS_LOAD_FFMPEG, &NWPVideoEditorView::OnLoadFFmpegFolder)
    ON_COMMAND(ID_FILE_NEW, &NWPVideoEditorView::OnFileNew)
    ON_WM_TIMER()
END_MESSAGE_MAP()

// ─────────────────────────────────────────────────────────────
// Construction / Destruction
// ─────────────────────────────────────────────────────────────

NWPVideoEditorView::NWPVideoEditorView()
{
    m_hPreviewBitmap = nullptr;
    m_previewTimePosition = 0.0;
}

NWPVideoEditorView::~NWPVideoEditorView()
{
    if (m_playbackTimer) {
        KillTimer(m_playbackTimer);
        m_playbackTimer = 0;
    }
    if (m_pDragImage) {
        delete m_pDragImage;
        m_pDragImage = nullptr;
    }
    if (m_hPreviewBitmap) {
        DeleteObject(m_hPreviewBitmap);
        m_hPreviewBitmap = nullptr;
    }
    for (auto& clip : m_timelineClips) {
        if (clip.hThumbnail) {
            DeleteObject(clip.hThumbnail);
            clip.hThumbnail = nullptr;
        }
    }
}

BOOL NWPVideoEditorView::PreCreateWindow(CREATESTRUCT& cs)
{
    return CView::PreCreateWindow(cs);
}

// ─────────────────────────────────────────────────────────────
// OnCreate
// ─────────────────────────────────────────────────────────────

int NWPVideoEditorView::OnCreate(LPCREATESTRUCT cs)
{
    if (CView::OnCreate(cs) == -1) return -1;

    CRect rc(0, 0, 100, 100);
    DWORD style = WS_CHILD | WS_VISIBLE | LVS_ICON | LVS_AUTOARRANGE | LVS_SINGLESEL;
    if (!m_list.Create(style, rc, this, 1001)) return -1;

    m_list.SetExtendedStyle(m_list.GetExtendedStyle()
        | LVS_EX_ONECLICKACTIVATE | LVS_EX_TWOCLICKACTIVATE);
    m_list.SetBkColor(RGB(45, 45, 45));
    m_list.SetTextBkColor(RGB(45, 45, 45));
    m_list.SetTextColor(RGB(220, 220, 220));

    if (!m_imgLarge.Create(96, 96, ILC_COLOR32 | ILC_MASK, 1, 32)) return -1;
    m_list.SetImageList(&m_imgLarge, LVSIL_NORMAL);

    m_rcPreview = CRect(0, 0, 720, 405);

    CString strText;
    strText.LoadString(IDS_BTN_PLAY);
    if (!m_playPauseButton.Create(strText, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        CRect(0, 0, 60, 25), this, ID_PLAY_PAUSE_BUTTON)) return -1;

    strText.LoadString(IDS_BTN_STOP);
    if (!m_stopButton.Create(strText, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        CRect(0, 0, 60, 25), this, ID_STOP_BUTTON)) return -1;

    strText.LoadString(IDS_BTN_ADD_TEXT);
    if (!m_addTextButton.Create(strText, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        CRect(0, 0, 70, 25), this, ID_ADD_TEXT_BUTTON)) return -1;

    CString savedPath = AfxGetApp()->GetProfileString(_T("Settings"), _T("FFmpegFolder"), _T(""));
    if (!savedPath.IsEmpty()) {
        m_config.SetFFmpegFolder(savedPath.GetString());
        InvalidateRect(&m_rcPreview, FALSE);
    }

    return 0;
}

// ─────────────────────────────────────────────────────────────
// OnDraw / OnSize / Layout
// ─────────────────────────────────────────────────────────────

void NWPVideoEditorView::OnDraw(CDC* pDC)
{
    CRect clientRect;
    GetClientRect(&clientRect);

    CDC memDC;
    CBitmap memBitmap;
    memDC.CreateCompatibleDC(pDC);
    memBitmap.CreateCompatibleBitmap(pDC, clientRect.Width(), clientRect.Height());
    CBitmap* oldBitmap = memDC.SelectObject(&memBitmap);

    memDC.FillSolidRect(&clientRect, RGB(45, 45, 45));
    DrawTimeline(&memDC);
    DrawPreviewFrame(&memDC);

    pDC->BitBlt(0, 0, clientRect.Width(), clientRect.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(oldBitmap);
}

void NWPVideoEditorView::OnSize(UINT nType, int cx, int cy)
{
    CView::OnSize(nType, cx, cy);
    Layout(cx, cy);
}

void NWPVideoEditorView::Layout(int cx, int cy)
{
    const int timelineH = 180;
    const int previewW = max(720, cx / 2);
    const int previewH = static_cast<int>(previewW * 9.0 / 16.0);
    const int margin = 10;
    const int buttonH = 25;
    const int buttonW = 60;
    const int addTextButtonW = 70;
    const int buttonSpacing = 10;

    if (m_list.GetSafeHwnd()) {
        int listWidth = cx - previewW - margin * 3;
        m_list.MoveWindow(margin, margin, max(120, listWidth),
            cy - timelineH - margin * 2);
    }

    int previewX = cx - previewW - margin;
    int previewY = margin;
    m_rcPreview = CRect(previewX, previewY, previewX + previewW, previewY + previewH);

    if (m_playPauseButton.GetSafeHwnd()) {
        int buttonY = previewY + previewH + 5;
        m_playPauseButton.MoveWindow(previewX, buttonY, buttonW, buttonH);
        m_stopButton.MoveWindow(previewX + buttonW + buttonSpacing, buttonY, buttonW, buttonH);
        m_addTextButton.MoveWindow(previewX + (buttonW + buttonSpacing) * 2,
            buttonY, addTextButtonW, buttonH);
    }

    m_rcTimeline = CRect(margin, cy - timelineH, cx - margin, cy - margin);
    Invalidate(FALSE);
}

// ─────────────────────────────────────────────────────────────
// On File - New
// ─────────────────────────────────────────────────────────────

void NWPVideoEditorView::OnFileNew()
{
    if (!m_timelineClips.empty() || !m_importedClips.empty() || !m_textOverlays.empty()) {
        if (AfxMessageBox(L"Start a new project? All unsaved changes will be lost.",
            MB_YESNO | MB_ICONQUESTION) != IDYES)
            return;
    }

    // Stop playback
    StopPlayback();

    // Clear timeline clips and their thumbnails
    for (auto& clip : m_timelineClips) {
        if (clip.hThumbnail) {
            DeleteObject(clip.hThumbnail);
            clip.hThumbnail = nullptr;
        }
    }
    m_timelineClips.clear();
    m_activeTimelineClipIndex = -1;
    m_contextMenuClipIndex = -1;

    // Clear text overlays
    m_textOverlays.clear();
    m_activeTextOverlayIndex = -1;

    // Clear imported clip list
    m_importedClips.clear();
    m_list.DeleteAllItems();
    m_imgLarge.DeleteImageList();
    m_imgLarge.Create(96, 96, ILC_COLOR32 | ILC_MASK, 1, 32);
    m_list.SetImageList(&m_imgLarge, LVSIL_NORMAL);

    // Clear preview
    ClearPreview();

    // Reset timeline state
    m_timelineDurationSec = 60.0;
    m_timelineScrollOffset = 0.0;
    m_dragState = DRAG_NONE;
    m_bDragging = FALSE;
    m_hoverTimePosition = -1.0;
    m_hoverClipIndex = -1;
    m_showSelectionBar = false;
    m_previewTimePosition = 0.0;

    Invalidate(FALSE);
}

// ─────────────────────────────────────────────────────────────
// FFmpeg folder
// ─────────────────────────────────────────────────────────────

void NWPVideoEditorView::OnLoadFFmpegFolder()
{
    BROWSEINFO bi = { 0 };
    bi.hwndOwner = GetSafeHwnd();
    bi.lpszTitle = L"Select FFmpeg Folder (containing ffmpeg.exe and ffprobe.exe)";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (!pidl) return;

    wchar_t folderPath[MAX_PATH];
    if (SHGetPathFromIDList(pidl, folderPath)) {
        m_config.SetFFmpegFolder(folderPath);

        if (m_config.IsFFmpegConfigured()) {
            CString msg;
            msg.Format(L"FFmpeg configured!\n\nFolder: %s", folderPath);
            AfxMessageBox(msg, MB_OK | MB_ICONINFORMATION);
            AfxGetApp()->WriteProfileString(_T("Settings"), _T("FFmpegFolder"), folderPath);
        }
        else {
            CString msg;
            msg.Format(L"Invalid FFmpeg folder!\n\nCould not find ffmpeg.exe and ffprobe.exe in:\n%s",
                folderPath);
            AfxMessageBox(msg, MB_OK | MB_ICONWARNING);
        }
    }
    CoTaskMemFree(pidl);
    InvalidateRect(&m_rcPreview, FALSE);
    UpdateWindow();
}

// ─────────────────────────────────────────────────────────────
// File import / export / save
// ─────────────────────────────────────────────────────────────

int NWPVideoEditorView::AddShellIconForFile(const CString& path)
{
    SHFILEINFO sfi;
    if (!SHGetFileInfo(path, FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi),
        SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES))
        return -1;
    int idx = m_imgLarge.Add(sfi.hIcon);
    if (sfi.hIcon) DestroyIcon(sfi.hIcon);
    return idx;
}

void NWPVideoEditorView::OnFileImport()
{
    const int BUFFER_SIZE = 100 * MAX_PATH + 2;
    CString fileBuffer;
    wchar_t* pBuffer = fileBuffer.GetBuffer(BUFFER_SIZE);
    memset(pBuffer, 0, BUFFER_SIZE * sizeof(wchar_t));

    CString filterStr;
    filterStr.LoadString(IDS_FILE_FILTER_MEDIA);
    CFileDialog dlg(TRUE, nullptr, nullptr,
        OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER, filterStr);
    dlg.m_ofn.lpstrFile = pBuffer;
    dlg.m_ofn.nMaxFile = BUFFER_SIZE;

    if (dlg.DoModal() != IDOK) { fileBuffer.ReleaseBuffer(); return; }
    fileBuffer.ReleaseBuffer();

    POSITION pos = dlg.GetStartPosition();
    std::vector<CString> selected;
    while (pos) selected.push_back(dlg.GetNextPathName(pos));

    for (size_t i = 0; i < selected.size(); i++) {
        const CString& fp = selected[i];
        ClipItem ci;
        ci.path = fp;
        ci.iImage = AddShellIconForFile(fp);
        ci.durationSec = GetVideoDurationAndSize(fp, m_videoWidth, m_videoHeight);
        m_importedClips.push_back(ci);

        CString name = fp;
        int s = name.ReverseFind(L'\\');
        if (s != -1) name = name.Mid(s + 1);

        int row = m_list.InsertItem(m_list.GetItemCount(), name, max(0, ci.iImage));
        if (i + 1 == selected.size())
            m_list.SetItemState(row, LVIS_SELECTED, LVIS_SELECTED);
    }

    UpdatePreview();

    if (selected.size() > 1) {
        CString fmt;
        fmt.LoadString(IDS_MSG_IMPORT_SUCCESS);
        CString msg;
        msg.Format(fmt, static_cast<int>(selected.size()));
        AfxMessageBox(msg);
    }
}

void NWPVideoEditorView::OnFileSave() { OnFileExport(); }

void NWPVideoEditorView::OnFileExport()
{
    if (!m_config.IsFFmpegConfigured()) {
        AfxMessageBox(
            L"FFmpeg is not configured!\nClick Settings > Load FFmpeg and select your FFmpeg folder.",
            MB_OK | MB_ICONWARNING);
        return;
    }
    if (m_timelineClips.empty()) {
        CString msg; msg.LoadString(IDS_MSG_ADD_CLIPS_FIRST);
        AfxMessageBox(msg); return;
    }

    CFileDialog sfd(FALSE, L"mp4", L"output.mp4", OFN_OVERWRITEPROMPT,
        L"MP4 video|*.mp4|All Files|*.*||");
    if (sfd.DoModal() != IDOK) return;

    CString out = sfd.GetPathName();
    if (out.ReverseFind(L'.') <= 0) out += L".mp4";

    std::vector<TimelineClip> sorted = m_timelineClips;
    std::sort(sorted.begin(), sorted.end(),
        [](const TimelineClip& a, const TimelineClip& b) {
            return a.startTimeOnTimeline < b.startTimeOnTimeline; });

    CString textFilter = BuildTextOverlayFilter();
    CString ffmpeg = m_config.GetFFmpegExePath().c_str();

    if (sorted.size() == 1) {
        const TimelineClip& c = sorted[0];
        CString cmd;
        if (!textFilter.IsEmpty())
            cmd.Format(L"\"%s\" -y -ss %.3f -i \"%s\" -t %.3f -vf %s "
                L"-c:v libx264 -preset veryfast -crf 20 -c:a aac \"%s\"",
                ffmpeg.GetString(), c.clipStartSec, c.path.GetString(),
                c.clipLengthSec, textFilter.GetString(), out.GetString());
        else
            cmd.Format(L"\"%s\" -y -ss %.3f -i \"%s\" -t %.3f "
                L"-c:v libx264 -preset veryfast -crf 20 -c:a aac \"%s\"",
                ffmpeg.GetString(), c.clipStartSec, c.path.GetString(),
                c.clipLengthSec, out.GetString());
        ExecuteFFmpegCommand(cmd);
    }
    else {
        CString cmd;
        cmd.Format(L"\"%s\" -y", ffmpeg.GetString());
        for (auto& c : sorted) {
            CString part;
            part.Format(L" -ss %.3f -t %.3f -i \"%s\"",
                c.clipStartSec, c.clipLengthSec, c.path.GetString());
            cmd += part;
        }

        CString fc = L" -filter_complex \"";
        for (int i = 0; i < (int)sorted.size(); i++) {
            CString seg; seg.Format(L"[%d:v][%d:a]", i, i);
            fc += seg;
        }
        CString cat;
        cat.Format(L"concat=n=%d:v=1:a=1[cv][ca]", (int)sorted.size());
        fc += cat;

        if (!textFilter.IsEmpty()) {
            fc += L";[cv]" + textFilter + L"[outv]\"";
            cmd += fc;
            cmd += L" -map [outv] -map [ca]";
        }
        else {
            fc += L"\"";
            cmd += fc;
            cmd += L" -map [cv] -map [ca]";
        }
        cmd += L" -c:v libx264 -preset veryfast -crf 20 -c:a aac \"" + out + L"\"";
        ExecuteFFmpegCommand(cmd);
    }
}

void NWPVideoEditorView::ExecuteFFmpegCommand(const CString& command)
{
    DWORD exit = 0;
    if (!RunProcessAndWait(command, INFINITE, &exit)) {
        CString msg; msg.LoadString(IDS_MSG_FFMPEG_NOT_FOUND);
        AfxMessageBox(msg); return;
    }
    if (exit == 0) {
        CString msg; msg.LoadString(IDS_MSG_EXPORT_SUCCESS);
        AfxMessageBox(msg);
    }
    else {
        CString msg; msg.LoadString(IDS_MSG_EXPORT_FAILED);
        AfxMessageBox(msg);
    }
}

BOOL NWPVideoEditorView::RunProcessAndWait(const CString& cmdLine, DWORD waitMS, DWORD* pExit)
{
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    CString mutable_cmd = cmdLine;
    LPWSTR psz = mutable_cmd.GetBuffer(mutable_cmd.GetLength() + 16);
    BOOL ok = CreateProcess(nullptr, psz, nullptr, nullptr, FALSE,
        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    mutable_cmd.ReleaseBuffer();
    if (!ok) return FALSE;
    WaitForSingleObject(pi.hProcess, waitMS);
    if (pExit) GetExitCodeProcess(pi.hProcess, pExit);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return TRUE;
}

// ─────────────────────────────────────────────────────────────
// Playback
// ─────────────────────────────────────────────────────────────

void NWPVideoEditorView::StartPlayback()
{
    if (m_activeTimelineClipIndex < 0 ||
        m_activeTimelineClipIndex >= (int)m_timelineClips.size()) return;

    m_isPlaying = true;
    m_playbackStartTime = m_previewTimePosition;
    m_playbackStartTick = GetTickCount();
    m_playbackTimer = SetTimer(1, 16, NULL);

    CString s; s.LoadString(IDS_BTN_PAUSE);
    m_playPauseButton.SetWindowText(s);
}

void NWPVideoEditorView::StopPlayback()
{
    if (m_playbackTimer) { KillTimer(m_playbackTimer); m_playbackTimer = 0; }
    m_isPlaying = false;
    CString s; s.LoadString(IDS_BTN_PLAY);
    m_playPauseButton.SetWindowText(s);
}

void NWPVideoEditorView::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1 && m_isPlaying) UpdatePlaybackFrame();
    CView::OnTimer(nIDEvent);
}

void NWPVideoEditorView::UpdatePlaybackFrame()
{
    if (m_activeTimelineClipIndex < 0 ||
        m_activeTimelineClipIndex >= (int)m_timelineClips.size())
    {
        StopPlayback(); return;
    }

    const TimelineClip& clip = m_timelineClips[m_activeTimelineClipIndex];
    double elapsed = (GetTickCount() - m_playbackStartTick) / 1000.0;
    double newTime = m_playbackStartTime + elapsed;
    double endTime = clip.clipStartSec + clip.clipLengthSec;

    if (newTime >= endTime - 0.0005) {
        newTime = endTime - 0.016;
        StopPlayback();
    }

    if (fabs(newTime - m_previewTimePosition) > 0.01) {
        m_previewTimePosition = newTime;
        LoadPreviewFrame(clip.path, m_previewTimePosition);
    }
}

void NWPVideoEditorView::OnPlayPause()
{
    if (m_isPlaying) StopPlayback();
    else             StartPlayback();
}

void NWPVideoEditorView::OnStop()
{
    StopPlayback();
    if (m_activeTimelineClipIndex >= 0 &&
        m_activeTimelineClipIndex < (int)m_timelineClips.size())
    {
        const TimelineClip& clip = m_timelineClips[m_activeTimelineClipIndex];
        m_previewTimePosition = clip.clipStartSec;
        LoadPreviewFrame(clip.path, m_previewTimePosition);
    }
}

// ─────────────────────────────────────────────────────────────
// Text overlays
// ─────────────────────────────────────────────────────────────

void NWPVideoEditorView::OnAddText()
{
    CTextInputDialog dlg(this);
    if (dlg.DoModal() != IDOK) return;

    TextOverlay overlay;
    overlay.text = dlg.GetText();
    overlay.startSec = 0.0;
    overlay.durSec = 5.0;
    overlay.position = CPoint(50, 50);
    m_textOverlays.push_back(overlay);
    m_activeTextOverlayIndex = (int)m_textOverlays.size() - 1;

    InvalidateRect(&m_rcTimeline, FALSE);
    InvalidateRect(&m_rcPreview, FALSE);

    CString fmt; fmt.LoadString(IDS_MSG_TEXT_ADDED);
    CString msg; msg.Format(fmt, overlay.text.GetString());
    AfxMessageBox(msg);
}

void NWPVideoEditorView::OnEditAddText() { OnAddText(); }

void NWPVideoEditorView::OnDeleteTextOverlay()
{
    if (m_activeTextOverlayIndex < 0 ||
        m_activeTextOverlayIndex >= (int)m_textOverlays.size()) return;

    CString fmt; fmt.LoadString(IDS_MSG_DELETE_TEXT);
    CString msg; msg.Format(fmt, m_textOverlays[m_activeTextOverlayIndex].text.GetString());

    if (AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION) == IDYES) {
        m_textOverlays.erase(m_textOverlays.begin() + m_activeTextOverlayIndex);
        m_activeTextOverlayIndex = -1;
        InvalidateRect(&m_rcTimeline, FALSE);
        InvalidateRect(&m_rcPreview, FALSE);
        CString deleted; deleted.LoadString(IDS_MSG_TEXT_DELETED);
        AfxMessageBox(deleted);
    }
}

void NWPVideoEditorView::DrawTextOverlays(CDC* pDC)
{
    double timelineTime = m_previewTimePosition;
    if (m_activeTimelineClipIndex >= 0 &&
        m_activeTimelineClipIndex < (int)m_timelineClips.size())
    {
        const TimelineClip& clip = m_timelineClips[m_activeTimelineClipIndex];
        timelineTime = clip.startTimeOnTimeline + (m_previewTimePosition - clip.clipStartSec);
    }
    CRect inner = m_rcPreview;
    inner.DeflateRect(2, 2);

    for (int i = 0; i < (int)m_textOverlays.size(); i++) {
        const TextOverlay& o = m_textOverlays[i];
        if (m_previewTimePosition < o.startSec ||
            m_previewTimePosition > o.startSec + o.durSec) continue;

        int sz = max(8, (int)(o.fontSize * m_prevScaleX));
        CFont font;
        font.CreateFont(sz, 0, 0, 0, FW_BOLD, FALSE, FALSE, 0,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS, o.fontName);

        CFont* oldFont = pDC->SelectObject(&font);
        pDC->SetBkMode(TRANSPARENT);

        int tx = inner.left + m_prevOffX + (int)(o.position.x * m_prevScaleX);
        int ty = inner.top + m_prevOffY + (int)(o.position.y * m_prevScaleY);

        pDC->SetTextColor(RGB(0, 0, 0));
        pDC->TextOut(tx + 2, ty + 2, o.text);
        pDC->SetTextColor(o.color);
        pDC->TextOut(tx, ty, o.text);
        pDC->SelectObject(oldFont);
    }
}

CString NWPVideoEditorView::BuildTextOverlayFilter() const
{
    if (m_textOverlays.empty()) return L"";
    CString filter;
    for (int i = 0; i < (int)m_textOverlays.size(); i++) {
        const TextOverlay& o = m_textOverlays[i];
        CString t = o.text;
        t.Replace(L"'", L"\\'");
        t.Replace(L":", L"\\:");
        int vx = max(0, min(o.position.x, m_videoWidth - 10));
        int vy = max(0, min(o.position.y, m_videoHeight - 10));
        CString f;
        f.Format(L"drawtext=text='%s':fontsize=%d:fontcolor=white:x=%d:y=%d"
            L":enable='between(t,%.2f,%.2f)'",
            t.GetString(), o.fontSize, vx, vy,
            o.startSec, o.startSec + o.durSec);
        filter = (i == 0) ? f : filter + L"," + f;
    }
    return filter;
}

// ─────────────────────────────────────────────────────────────
// Context menu
// ─────────────────────────────────────────────────────────────

void NWPVideoEditorView::OnContextMenu(CWnd*, CPoint point)
{
    ScreenToClient(&point);

    int textHit = HitTestTimelineTextOverlay(point);
    if (textHit >= 0) {
        m_activeTextOverlayIndex = textHit;
        CMenu menu;
        menu.CreatePopupMenu();
        CString fmt; fmt.LoadString(IDS_MENU_DELETE);
        CString label; label.Format(fmt, m_textOverlays[textHit].text.GetString());
        menu.AppendMenu(MF_STRING, 32775, label);
        CPoint sp = point; ClientToScreen(&sp);
        menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, sp.x, sp.y, this);
        return;
    }

    if (m_rcTimeline.PtInRect(point))
        OnRButtonDown(0, point);
}

// ─────────────────────────────────────────────────────────────
// Mouse handling
// ─────────────────────────────────────────────────────────────

void NWPVideoEditorView::OnLButtonDown(UINT nFlags, CPoint pt)
{
    // Text overlay drag in preview
    int previewTextHit = HitTestTextOverlayInPreview(pt);
    if (previewTextHit >= 0) {
        m_draggingTextIndex = previewTextHit;
        m_activeTextOverlayIndex = previewTextHit;
        m_dragState = DRAG_TEXT_PREVIEW;

        CRect inner = m_rcPreview; inner.DeflateRect(2, 2);
        int cx = inner.left + m_prevOffX +
            (int)(m_textOverlays[previewTextHit].position.x * m_prevScaleX);
        int cy = inner.top + m_prevOffY +
            (int)(m_textOverlays[previewTextHit].position.y * m_prevScaleY);
        m_textDragStart = CPoint(pt.x - cx, pt.y - cy);
        SetCapture();
        InvalidateRect(&m_rcPreview, FALSE);
        return;
    }

    // Text overlay handles on timeline
    int timelineTextHit = HitTestTimelineTextOverlay(pt);
    if (timelineTextHit >= 0) {
        m_activeTextOverlayIndex = timelineTextHit;
        int handleHit = HitTestTextOverlayHandles(pt);

        if (handleHit == 1) {
            m_dragState = DRAG_TEXT_LEFT_HANDLE;
            m_dragStart = pt;
            m_dragStartTextStart = m_textOverlays[timelineTextHit].startSec;
            m_dragStartTextDur = m_textOverlays[timelineTextHit].durSec;
            SetCapture();
        }
        else if (handleHit == 2) {
            m_dragState = DRAG_TEXT_RIGHT_HANDLE;
            m_dragStart = pt;
            m_dragStartTextStart = m_textOverlays[timelineTextHit].startSec;
            m_dragStartTextDur = m_textOverlays[timelineTextHit].durSec;
            SetCapture();
        }
        else {
            m_dragState = DRAG_TEXT_TIMELINE;
            m_dragStart = pt;
            m_dragStartTimelinePos = m_textOverlays[timelineTextHit].startSec;
            SetCapture();
        }
        InvalidateRect(&m_rcTimeline, FALSE);
        return;
    }

    if (!m_rcTimeline.PtInRect(pt)) { CView::OnLButtonDown(nFlags, pt); return; }

    m_dragState = DRAG_NONE;
    int hitIdx = HitTestTimelineClip(pt);

    if (hitIdx >= 0) {
        m_activeTimelineClipIndex = hitIdx;

        if (m_hoverTimePosition >= 0.0 && m_hoverClipIndex == hitIdx) {
            m_previewTimePosition = m_hoverTimePosition;
            LoadPreviewFrame(m_timelineClips[hitIdx].path, m_previewTimePosition);
        }
        else {
            UpdatePreview();
        }

        const TimelineClip& clip = m_timelineClips[hitIdx];
        int x1 = SecondsToTimelineX(clip.startTimeOnTimeline);
        int x2 = SecondsToTimelineX(clip.startTimeOnTimeline + clip.clipLengthSec);

        if (pt.x >= x1 && pt.x <= x1 + 8) {
            m_dragState = DRAG_LEFT_HANDLE;
            m_dragStart = pt;
            m_dragStartClipStart = clip.clipStartSec;
            m_dragStartClipLength = clip.clipLengthSec;
            m_dragStartTimelinePos = clip.startTimeOnTimeline;
            SetCapture();
        }
        else if (pt.x >= x2 - 8 && pt.x <= x2) {
            m_dragState = DRAG_RIGHT_HANDLE;
            m_dragStart = pt;
            m_dragStartClipStart = clip.clipStartSec;
            m_dragStartClipLength = clip.clipLengthSec;
            SetCapture();
        }
        else {
            m_dragState = DRAG_CLIP_MOVE;
            m_dragStart = pt;
            m_dragStartTimelinePos = clip.startTimeOnTimeline;
            SetCapture();
        }
        InvalidateRect(&m_rcTimeline, FALSE);
    }
    else {
        m_activeTimelineClipIndex = -1;
        UpdatePreview();
        InvalidateRect(&m_rcTimeline, FALSE);
    }
}

void NWPVideoEditorView::OnMouseMove(UINT nFlags, CPoint pt)
{
    // File list drag
    if (m_bDragging && m_pDragImage) {
        CPoint sp(pt); ClientToScreen(&sp);
        m_pDragImage->DragMove(sp);
        SetCursor(LoadCursor(NULL, IsOverTimeline(sp) ? IDC_ARROW : IDC_NO));
        return;
    }

    // Text overlay in preview drag
    if (m_dragState == DRAG_TEXT_PREVIEW && GetCapture() == this
        && m_draggingTextIndex >= 0) {
        CRect inner = m_rcPreview; inner.DeflateRect(2, 2);
        m_textOverlays[m_draggingTextIndex].position.x =
            (int)((pt.x - m_textDragStart.x - inner.left - m_prevOffX) / m_prevScaleX);
        m_textOverlays[m_draggingTextIndex].position.y =
            (int)((pt.y - m_textDragStart.y - inner.top - m_prevOffY) / m_prevScaleY);
        InvalidateRect(&m_rcPreview, FALSE);
        return;
    }

    // Text handle drag
    if ((m_dragState == DRAG_TEXT_LEFT_HANDLE || m_dragState == DRAG_TEXT_RIGHT_HANDLE)
        && m_activeTextOverlayIndex >= 0 && GetCapture() == this)
    {
        TextOverlay& o = m_textOverlays[m_activeTextOverlayIndex];
        double delta = TimelineXToSeconds(pt.x) - TimelineXToSeconds(m_dragStart.x);
        if (m_dragState == DRAG_TEXT_LEFT_HANDLE) {
            o.startSec = max(0.0, m_dragStartTextStart + delta);
            o.durSec = max(0.5, m_dragStartTextDur - delta);
        }
        else {
            o.durSec = max(0.5, m_dragStartTextDur + delta);
        }
        InvalidateRect(&m_rcTimeline, FALSE);
        return;
    }

    // Text timeline move
    if (m_dragState == DRAG_TEXT_TIMELINE && GetCapture() == this
        && m_activeTextOverlayIndex >= 0 &&
        m_activeTextOverlayIndex < (int)m_textOverlays.size())
    {
        double delta = TimelineXToSeconds(pt.x) - TimelineXToSeconds(m_dragStart.x);
        m_textOverlays[m_activeTextOverlayIndex].startSec =
            max(0.0, m_dragStartTimelinePos + delta);
        InvalidateRect(&m_rcTimeline, FALSE);
        return;
    }

    // Clip drag
    if (m_dragState != DRAG_NONE && GetCapture() == this
        && m_activeTimelineClipIndex >= 0 &&
        m_activeTimelineClipIndex < (int)m_timelineClips.size())
    {
        TimelineClip& clip = m_timelineClips[m_activeTimelineClipIndex];

        if (m_dragState == DRAG_LEFT_HANDLE) {
            double delta = TimelineXToSeconds(pt.x) - TimelineXToSeconds(m_dragStart.x);
            double newTL = max(0.0, m_dragStartTimelinePos + delta);
            double newCS = max(0.0, min(clip.originalDuration - 1.0,
                m_dragStartClipStart - delta));
            double rightEdge = m_dragStartTimelinePos + m_dragStartClipLength;
            double newLen = max(1.0, rightEdge - newTL);
            if (newCS + newLen > clip.originalDuration)
                newLen = clip.originalDuration - newCS;
            if (newLen < 1.0) { newLen = 1.0; newCS = max(0.0, clip.originalDuration - 1.0); }
            clip.startTimeOnTimeline = newTL;
            clip.clipStartSec = newCS;
            clip.clipLengthSec = newLen;
        }
        else if (m_dragState == DRAG_RIGHT_HANDLE) {
            double delta = TimelineXToSeconds(pt.x) - TimelineXToSeconds(m_dragStart.x);
            double maxLen = clip.originalDuration - clip.clipStartSec;
            clip.clipLengthSec = max(1.0, min(maxLen, m_dragStartClipLength + delta));
        }
        else if (m_dragState == DRAG_CLIP_MOVE) {
            double delta = TimelineXToSeconds(pt.x) - TimelineXToSeconds(m_dragStart.x);
            double newPos = max(0.0, m_dragStartTimelinePos + delta);

            for (int i = 0; i < (int)m_timelineClips.size(); i++) {
                if (i == m_activeTimelineClipIndex) continue;
                const TimelineClip& other = m_timelineClips[i];
                double os = other.startTimeOnTimeline;
                double oe = os + other.clipLengthSec;
                double ts = newPos, te = newPos + clip.clipLengthSec;
                if (!(te <= os || ts >= oe)) {
                    newPos = (delta > 0) ? oe : max(0.0, os - clip.clipLengthSec);
                }
            }
            clip.startTimeOnTimeline = newPos;
        }
        InvalidateRect(&m_rcTimeline, FALSE);
        return;
    }

    // Hover on timeline
    if (m_rcTimeline.PtInRect(pt) && m_dragState == DRAG_NONE) {
        int hitIdx = HitTestTimelineClip(pt);
        if (hitIdx >= 0) {
            const TimelineClip& clip = m_timelineClips[hitIdx];
            int cx1 = SecondsToTimelineX(clip.startTimeOnTimeline);
            int cx2 = SecondsToTimelineX(clip.startTimeOnTimeline + clip.clipLengthSec);
            double ratio = max(0.0, min(1.0,
                (double)(pt.x - cx1) / max(1, cx2 - cx1)));
            double hover = clip.clipStartSec + ratio * clip.clipLengthSec;

            bool changed = fabs(hover - m_hoverTimePosition) > 0.1
                || m_hoverClipIndex != hitIdx
                || !m_showSelectionBar;

            if (changed) {
                m_hoverTimePosition = hover;
                m_hoverClipIndex = hitIdx;
                m_showSelectionBar = true;

                if (!m_isPlaying && hitIdx == m_activeTimelineClipIndex) {
                    DWORD now = GetTickCount();
                    if (now - m_lastPreviewUpdateTick > 150) {
                        LoadPreviewFrame(clip.path, hover);
                        m_lastPreviewUpdateTick = now;
                    }
                }
                InvalidateRect(&m_rcTimeline, FALSE);
            }
        }
        else if (m_showSelectionBar) {
            m_showSelectionBar = false;
            m_hoverClipIndex = -1;
            m_hoverTimePosition = -1.0;
            InvalidateRect(&m_rcTimeline, FALSE);
        }
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    }
    else if (m_showSelectionBar) {
        m_showSelectionBar = false;
        m_hoverClipIndex = -1;
        m_hoverTimePosition = -1.0;
        InvalidateRect(&m_rcTimeline, FALSE);
    }

    if (m_rcPreview.PtInRect(pt) && HitTestTextOverlayInPreview(pt) >= 0)
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));
}

void NWPVideoEditorView::OnLButtonUp(UINT nFlags, CPoint pt)
{
    if (m_dragState == DRAG_TEXT_PREVIEW) {
        if (GetCapture() == this) ReleaseCapture();
        m_dragState = DRAG_NONE;
        m_draggingTextIndex = -1;
        InvalidateRect(&m_rcPreview, FALSE);
        return;
    }

    if (m_bDragging) {
        if (GetCapture() == this) ReleaseCapture();
        m_bDragging = FALSE;
        if (m_pDragImage) {
            m_pDragImage->DragLeave(GetDesktopWindow());
            m_pDragImage->EndDrag();
            delete m_pDragImage;
            m_pDragImage = nullptr;
        }
        CPoint sp(pt); ClientToScreen(&sp);
        if (IsOverTimeline(sp) && m_nDragIndex >= 0 &&
            m_nDragIndex < (int)m_importedClips.size())
            AddClipToTimeline(m_importedClips[m_nDragIndex].path);
        m_nDragIndex = -1;
        return;
    }

    if (m_dragState != DRAG_NONE) {
        if (GetCapture() == this) ReleaseCapture();
        m_dragState = DRAG_NONE;
        InvalidateRect(&m_rcTimeline, FALSE);
    }
    CView::OnLButtonUp(nFlags, pt);
}

void NWPVideoEditorView::OnRButtonDown(UINT nFlags, CPoint pt)
{
    if (!m_rcTimeline.PtInRect(pt)) { CView::OnRButtonDown(nFlags, pt); return; }

    int hitIdx = HitTestTimelineClip(pt);
    if (hitIdx < 0) { CView::OnRButtonDown(nFlags, pt); return; }

    m_contextMenuClipIndex = hitIdx;
    m_activeTimelineClipIndex = hitIdx;
    InvalidateRect(&m_rcTimeline, FALSE);

    CString clipName = m_timelineClips[hitIdx].path;
    int s = clipName.ReverseFind(L'\\');
    if (s != -1) clipName = clipName.Mid(s + 1);

    CMenu menu;
    menu.CreatePopupMenu();

    CString splitFmt, splitLabel;
    if (m_showSelectionBar && m_hoverClipIndex == hitIdx) {
        splitFmt.LoadString(IDS_MENU_SPLIT_AT);
        splitLabel.Format(splitFmt, clipName.GetString(), m_hoverTimePosition);
    }
    else {
        splitFmt.LoadString(IDS_MENU_SPLIT);
        splitLabel.Format(splitFmt, clipName.GetString());
    }
    menu.AppendMenu(MF_STRING, ID_TIMELINE_SPLIT_CLIP, splitLabel);
    menu.AppendMenu(MF_SEPARATOR);

    CString removeFmt, removeLabel;
    removeFmt.LoadString(IDS_MENU_REMOVE_FROM_TIMELINE);
    removeLabel.Format(removeFmt, clipName.GetString());
    menu.AppendMenu(MF_STRING, ID_TIMELINE_REMOVE_CLIP, removeLabel);

    CPoint sp = pt; ClientToScreen(&sp);
    menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, sp.x, sp.y, this);
}

// ─────────────────────────────────────────────────────────────
// Timeline clip operations
// ─────────────────────────────────────────────────────────────

void NWPVideoEditorView::OnTimelineRemoveClip()
{
    if (m_contextMenuClipIndex < 0 ||
        m_contextMenuClipIndex >= (int)m_timelineClips.size()) return;

    CString name = m_timelineClips[m_contextMenuClipIndex].path;
    int s = name.ReverseFind(L'\\'); if (s != -1) name = name.Mid(s + 1);

    CString fmt; fmt.LoadString(IDS_MSG_REMOVE_CLIP);
    CString msg; msg.Format(fmt, name.GetString());

    if (AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION) != IDYES)
    {
        m_contextMenuClipIndex = -1; return;
    }

    if (m_timelineClips[m_contextMenuClipIndex].hThumbnail) {
        DeleteObject(m_timelineClips[m_contextMenuClipIndex].hThumbnail);
        m_timelineClips[m_contextMenuClipIndex].hThumbnail = nullptr;
    }
    m_timelineClips.erase(m_timelineClips.begin() + m_contextMenuClipIndex);

    if (m_activeTimelineClipIndex == m_contextMenuClipIndex)
        m_activeTimelineClipIndex = -1;
    else if (m_activeTimelineClipIndex > m_contextMenuClipIndex)
        m_activeTimelineClipIndex--;

    RepositionClipsAfterRemoval();
    UpdatePreview();
    InvalidateRect(&m_rcTimeline, FALSE);
    m_contextMenuClipIndex = -1;
}

void NWPVideoEditorView::OnTimelineSplitClip()
{
    if (m_contextMenuClipIndex < 0 ||
        m_contextMenuClipIndex >= (int)m_timelineClips.size()) return;

    TimelineClip orig = m_timelineClips[m_contextMenuClipIndex];
    double splitTime = (m_showSelectionBar && m_hoverClipIndex == m_contextMenuClipIndex)
        ? m_hoverTimePosition
        : orig.clipStartSec + orig.clipLengthSec / 2.0;

    if (splitTime <= orig.clipStartSec ||
        splitTime >= orig.clipStartSec + orig.clipLengthSec)
    {
        CString msg; msg.LoadString(IDS_MSG_INVALID_SPLIT);
        AfxMessageBox(msg); m_contextMenuClipIndex = -1; return;
    }

    TimelineClip second = orig;
    second.startTimeOnTimeline = orig.startTimeOnTimeline + (splitTime - orig.clipStartSec);
    second.clipStartSec = splitTime;
    second.clipLengthSec = orig.clipStartSec + orig.clipLengthSec - splitTime;
    second.hThumbnail = nullptr;

    m_timelineClips[m_contextMenuClipIndex].clipLengthSec = splitTime - orig.clipStartSec;
    m_timelineClips.insert(m_timelineClips.begin() + m_contextMenuClipIndex + 1, second);

    m_activeTimelineClipIndex = m_contextMenuClipIndex;
    m_showSelectionBar = false;
    m_hoverClipIndex = -1;
    m_hoverTimePosition = -1.0;

    UpdatePreview();
    InvalidateRect(&m_rcTimeline, FALSE);
    m_contextMenuClipIndex = -1;
}

void NWPVideoEditorView::RepositionClipsAfterRemoval()
{
    std::sort(m_timelineClips.begin(), m_timelineClips.end(),
        [](const TimelineClip& a, const TimelineClip& b) {
            return a.startTimeOnTimeline < b.startTimeOnTimeline; });
    double pos = 0.0;
    for (auto& c : m_timelineClips) { c.startTimeOnTimeline = pos; pos += c.clipLengthSec; }
}

void NWPVideoEditorView::AddClipToTimeline(const CString& clipPath)
{
    // Get real duration first
    int w = 480, h = 360;
    double realDuration = GetVideoDurationAndSize(clipPath, w, h);
    m_videoWidth = w;
    m_videoHeight = h;

    int imgIdx = -1;
    for (const auto& ci : m_importedClips) {
        if (ci.path == clipPath) { imgIdx = ci.iImage; break; }
    }

    double start = 0.0;
    for (const auto& c : m_timelineClips) {
        double e = c.startTimeOnTimeline + c.clipLengthSec;
        if (e > start) start = e;
    }

    TimelineClip nc;
    nc.path = clipPath;
    nc.startTimeOnTimeline = start;
    nc.clipStartSec = 0.0;
    nc.clipLengthSec = realDuration;
    nc.originalDuration = realDuration;
    nc.iImage = imgIdx;
    nc.hThumbnail = ExtractThumbnail(clipPath, 1.0);
    m_timelineClips.push_back(nc);
    m_activeTimelineClipIndex = (int)m_timelineClips.size() - 1;

    UpdatePreview();
    m_dragState = DRAG_NONE;
    m_bDragging = FALSE;
    InvalidateRect(&m_rcTimeline, FALSE);
}

// ─────────────────────────────────────────────────────────────
// List control callbacks
// ─────────────────────────────────────────────────────────────

void NWPVideoEditorView::OnListDblClk(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMITEMACTIVATE* p = (NMITEMACTIVATE*)pNMHDR;
    if (p->iItem >= 0 && p->iItem < (int)m_importedClips.size())
        AddClipToTimeline(m_importedClips[p->iItem].path);
    if (pResult) *pResult = 0;
}

void NWPVideoEditorView::OnListItemActivate(NMHDR*, LRESULT* pResult)
{
    int sel = m_list.GetNextItem(-1, LVNI_SELECTED);
    if (sel >= 0 && sel < (int)m_importedClips.size()) {
        GetVideoDurationAndSize(m_importedClips[sel].path, m_videoWidth, m_videoHeight);
        m_previewTimePosition = 0.0;
        LoadPreviewFrame(m_importedClips[sel].path, 0.0);
    }
    if (pResult) *pResult = 0;
}

void NWPVideoEditorView::OnListBeginDrag(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMLISTVIEW* p = (NMLISTVIEW*)pNMHDR;
    m_nDragIndex = p->iItem;
    if (m_nDragIndex >= 0 && m_nDragIndex < (int)m_importedClips.size()) {
        POINT pt = { 10, 10 };
        m_pDragImage = m_list.CreateDragImage(m_nDragIndex, &pt);
        if (m_pDragImage) {
            m_pDragImage->BeginDrag(0, CPoint(10, 10));
            m_pDragImage->DragEnter(GetDesktopWindow(), p->ptAction);
            m_bDragging = TRUE;
            SetCapture();
        }
    }
    *pResult = 0;
}

// ─────────────────────────────────────────────────────────────
// Hit testing
// ─────────────────────────────────────────────────────────────

int NWPVideoEditorView::HitTestTimelineClip(CPoint pt) const
{
    int top = m_rcTimeline.top + 48;
    int bottom = m_rcTimeline.bottom - 56;
    for (int i = 0; i < (int)m_timelineClips.size(); i++) {
        const TimelineClip& c = m_timelineClips[i];
        CRect r(SecondsToTimelineX(c.startTimeOnTimeline), top,
            SecondsToTimelineX(c.startTimeOnTimeline + c.clipLengthSec), bottom);
        if (r.PtInRect(pt)) return i;
    }
    return -1;
}

int NWPVideoEditorView::HitTestTimelineTextOverlay(CPoint pt) const
{
    if (!m_rcTimeline.PtInRect(pt)) return -1;
    int top = m_rcTimeline.bottom - 50;
    int bottom = m_rcTimeline.bottom - 10;
    if (pt.y < top || pt.y > bottom) return -1;
    for (int i = 0; i < (int)m_textOverlays.size(); i++) {
        int sx = SecondsToTimelineX(m_textOverlays[i].startSec);
        int ex = SecondsToTimelineX(m_textOverlays[i].startSec + m_textOverlays[i].durSec);
        if (pt.x >= sx && pt.x <= ex) return i;
    }
    return -1;
}

int NWPVideoEditorView::HitTestTextOverlayHandles(CPoint pt) const
{
    if (m_activeTextOverlayIndex < 0 ||
        m_activeTextOverlayIndex >= (int)m_textOverlays.size()) return 0;
    const TextOverlay& o = m_textOverlays[m_activeTextOverlayIndex];
    int top = m_rcTimeline.bottom - 50;
    int bottom = m_rcTimeline.bottom - 10;
    int sx = SecondsToTimelineX(o.startSec);
    int ex = SecondsToTimelineX(o.startSec + o.durSec);
    if (pt.x >= sx && pt.x <= sx + 8 && pt.y >= top && pt.y <= bottom) return 1;
    if (pt.x >= ex - 8 && pt.x <= ex && pt.y >= top && pt.y <= bottom) return 2;
    return 0;
}

int NWPVideoEditorView::HitTestTextOverlayInPreview(CPoint pt) const
{
    if (!m_rcPreview.PtInRect(pt)) return -1;
    CRect inner = m_rcPreview; inner.DeflateRect(2, 2);
    CPoint rel(pt.x - inner.left, pt.y - inner.top);
    for (int i = (int)m_textOverlays.size() - 1; i >= 0; i--) {
        const TextOverlay& o = m_textOverlays[i];
        CRect r(o.position.x - 10, o.position.y - 10,
            o.position.x + 200, o.position.y + 40);
        if (r.PtInRect(rel)) return i;
    }
    return -1;
}

// ─────────────────────────────────────────────────────────────
// Preview
// ─────────────────────────────────────────────────────────────

void NWPVideoEditorView::UpdatePreview()
{
    StopPlayback();
    if (m_activeTimelineClipIndex >= 0 &&
        m_activeTimelineClipIndex < (int)m_timelineClips.size())
    {
        const TimelineClip& c = m_timelineClips[m_activeTimelineClipIndex];
        GetVideoDurationAndSize(c.path, m_videoWidth, m_videoHeight);
        m_previewTimePosition = c.clipStartSec;
        LoadPreviewFrame(c.path, m_previewTimePosition);
    }
    else {
        int sel = m_list.GetNextItem(-1, LVNI_SELECTED);
        if (sel >= 0 && sel < (int)m_importedClips.size()) {
            GetVideoDurationAndSize(m_importedClips[sel].path, m_videoWidth, m_videoHeight);
            m_previewTimePosition = 0.0;
            LoadPreviewFrame(m_importedClips[sel].path, 0.0);
        }
        else {
            ClearPreview();
        }
    }
}

void NWPVideoEditorView::LoadPreviewFrame(const CString& filePath, double timePosition)
{
    if (m_hPreviewBitmap) { DeleteObject(m_hPreviewBitmap); m_hPreviewBitmap = nullptr; }

    if (!m_config.IsFFmpegConfigured()) {
        CreateTestFrame(filePath, timePosition);
        InvalidateRect(&m_rcPreview, FALSE);
        return;
    }

    wchar_t tmp[MAX_PATH];
    GetTempPathW(MAX_PATH, tmp);

    CString framePath;
    framePath.Format(L"%snwpframe_%u.bmp", tmp, GetCurrentProcessId());

    // Delete any leftover from previous call
    DeleteFileW(framePath);

    CString cmd;
    cmd.Format(L"\"%s\" -y -ss %.3f -i \"%s\" -vframes 1 -f image2 \"%s\"",
        m_config.GetFFmpegExePath().c_str(),
        timePosition,
        filePath.GetString(),
        framePath.GetString());

    DWORD exit = 0;
    RunProcessAndWait(cmd, 8000, &exit);

    if (exit == 0 && GetFileAttributesW(framePath) != INVALID_FILE_ATTRIBUTES) {
        LoadBitmapFromFile(framePath);
        DeleteFileW(framePath);
    }

    if (!m_hPreviewBitmap)
        CreateTestFrame(filePath, timePosition);

    if (m_hPreviewBitmap) {
        CRect inner = m_rcPreview; inner.DeflateRect(2, 2);
        double sx = (double)inner.Width() / max(1, m_videoWidth);
        double sy = (double)inner.Height() / max(1, m_videoHeight);
        double sc = min(sx, sy);
        m_prevScaleX = sc; m_prevScaleY = sc;
        m_prevOffX = (inner.Width() - (int)(m_videoWidth * sc)) / 2;
        m_prevOffY = (inner.Height() - (int)(m_videoHeight * sc)) / 2;
    }
    InvalidateRect(&m_rcPreview, FALSE);
}

void NWPVideoEditorView::DrawPreviewFrame(CDC* pDC)
{
    if (m_rcPreview.IsRectEmpty()) return;
    pDC->FillSolidRect(&m_rcPreview, RGB(20, 20, 20));

    CRect inner = m_rcPreview; inner.DeflateRect(2, 2);

    if (m_hPreviewBitmap) {
        CDC memDC; memDC.CreateCompatibleDC(pDC);
        HBITMAP old = (HBITMAP)memDC.SelectObject(m_hPreviewBitmap);
        BITMAP bm;
        if (GetObject(m_hPreviewBitmap, sizeof(bm), &bm) > 0) {
            double sx = (double)inner.Width() / max(1, bm.bmWidth);
            double sy = (double)inner.Height() / max(1, bm.bmHeight);
            double sc = min(sx, sy);
            int dw = (int)(bm.bmWidth * sc), dh = (int)(bm.bmHeight * sc);
            int ox = inner.left + (inner.Width() - dw) / 2;
            int oy = inner.top + (inner.Height() - dh) / 2;
            pDC->SetStretchBltMode(HALFTONE);
            pDC->StretchBlt(ox, oy, dw, dh, &memDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
        }
        memDC.SelectObject(old);
    }
    else {
        pDC->SetTextColor(RGB(120, 120, 120));
        pDC->SetBkMode(TRANSPARENT);
        pDC->DrawText(_T("No Preview"), &inner, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    DrawTextOverlays(pDC);

    CPen pen(PS_SOLID, 2, RGB(80, 80, 80));
    CPen* old = pDC->SelectObject(&pen);
    pDC->SelectStockObject(NULL_BRUSH);
    pDC->Rectangle(&m_rcPreview);
    pDC->SelectObject(old);
}

// ─────────────────────────────────────────────────────────────
// Timeline drawing
// ─────────────────────────────────────────────────────────────

void NWPVideoEditorView::DrawTimeline(CDC* pDC)
{
    if (m_rcTimeline.IsRectEmpty()) return;
    pDC->FillSolidRect(&m_rcTimeline, RGB(32, 32, 32));
    pDC->Draw3dRect(&m_rcTimeline, RGB(80, 80, 80), RGB(40, 40, 40));

    const CRect& r = m_rcTimeline;
    int trackTop = r.top + 48;
    int trackBottom = r.bottom - 56;

    // Grid lines
    CPen gridPen(PS_SOLID, 1, RGB(60, 60, 60));
    CPen* oldPen = pDC->SelectObject(&gridPen);
    CFont timeFont;
    timeFont.CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_SWISS, _T("Segoe UI"));
    CFont* oldFont = pDC->SelectObject(&timeFont);
    pDC->SetTextColor(RGB(180, 180, 180));
    pDC->SetBkMode(TRANSPARENT);

    for (int s = 0; s <= (int)m_timelineDurationSec; s += 5) {
        int x = SecondsToTimelineX((double)s);
        pDC->MoveTo(x, r.top + 25); pDC->LineTo(x, r.bottom - 8);
        CString lbl; lbl.Format(_T("%ds"), s);
        CRect tr(x - 20, r.top + 5, x + 20, r.top + 22);
        pDC->DrawText(lbl, tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    pDC->SelectObject(oldFont);
    pDC->SelectObject(oldPen);

    // Clips
    for (int i = 0; i < (int)m_timelineClips.size(); i++) {
        const TimelineClip& c = m_timelineClips[i];
        int x1 = SecondsToTimelineX(c.startTimeOnTimeline);
        int x2 = SecondsToTimelineX(c.startTimeOnTimeline + c.clipLengthSec);
        CRect cr(x1, trackTop, x2, trackBottom);

        pDC->FillSolidRect(cr,
            (i == m_activeTimelineClipIndex) ? RGB(90, 150, 230) : RGB(70, 100, 150));

        // Thumbnail
        if (c.hThumbnail && cr.Width() > 40) {
            CDC mdc; mdc.CreateCompatibleDC(pDC);
            HBITMAP ob = (HBITMAP)mdc.SelectObject(c.hThumbnail);
            BITMAP bm;
            if (GetObject(c.hThumbnail, sizeof(bm), &bm) > 0) {
                int tw = min(cr.Width() / 2, 160), th = cr.Height() - 8;
                pDC->SetStretchBltMode(HALFTONE);
                pDC->StretchBlt(cr.left + 4, cr.top + 4, tw, th,
                    &mdc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
            }
            mdc.SelectObject(ob);
        }

        CRect hi(cr.left, cr.top, cr.right, cr.top + 2);
        pDC->FillSolidRect(hi, RGB(120, 180, 255));

        CPen bp(PS_SOLID, 1, RGB(50, 50, 50)); CPen* op = pDC->SelectObject(&bp);
        pDC->SelectStockObject(NULL_BRUSH); pDC->Rectangle(cr);
        pDC->SelectObject(op);

        CRect lh(cr.left, cr.top, cr.left + 8, cr.bottom);
        CRect rh(cr.right - 8, cr.top, cr.right, cr.bottom);
        pDC->FillSolidRect(lh, RGB(220, 220, 220));
        pDC->FillSolidRect(rh, RGB(220, 220, 220));

        CString name = c.path;
        int sl = name.ReverseFind(L'\\'); if (sl != -1) name = name.Mid(sl + 1);
        pDC->SetBkMode(TRANSPARENT); pDC->SetTextColor(RGB(255, 255, 255));
        CRect tr = cr; tr.DeflateRect(10, 4);
        pDC->DrawText(name, tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    // Text overlay track
    CRect textTrack(r.left + 10, r.bottom - 50, r.right - 10, r.bottom - 10);
    pDC->FillSolidRect(textTrack, RGB(40, 40, 40));
    pDC->SetTextColor(RGB(150, 150, 150)); pDC->SetBkMode(TRANSPARENT);
    CRect ll(r.left + 12, textTrack.top, r.left + 60, textTrack.bottom);
    pDC->DrawText(_T("TEXT"), ll, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    CPen tbp(PS_SOLID, 1, RGB(80, 80, 80)); CPen* otp = pDC->SelectObject(&tbp);
    pDC->SelectStockObject(NULL_BRUSH); pDC->Rectangle(textTrack);
    pDC->SelectObject(otp);

    for (int i = 0; i < (int)m_textOverlays.size(); i++) {
        const TextOverlay& t = m_textOverlays[i];
        int sx = SecondsToTimelineX(t.startSec);
        int ex = SecondsToTimelineX(t.startSec + t.durSec);
        CRect tr(sx, textTrack.top + 4, ex, textTrack.bottom - 4);
        pDC->FillSolidRect(tr,
            (i == m_activeTextOverlayIndex) ? RGB(220, 160, 90) : RGB(180, 130, 70));

        CPen tbp2(PS_SOLID, 1, RGB(100, 70, 40)); CPen* op2 = pDC->SelectObject(&tbp2);
        pDC->SelectStockObject(NULL_BRUSH); pDC->Rectangle(tr);
        pDC->SelectObject(op2);

        pDC->FillSolidRect(CRect(tr.left, tr.top, tr.left + 8, tr.bottom), RGB(230, 230, 230));
        pDC->FillSolidRect(CRect(tr.right - 8, tr.top, tr.right, tr.bottom), RGB(230, 230, 230));

        pDC->SetBkMode(TRANSPARENT); pDC->SetTextColor(RGB(15, 15, 15));
        CRect tlr = tr; tlr.DeflateRect(10, 2);
        pDC->DrawText(t.text, tlr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    // Hover selection bar
    if (m_showSelectionBar && m_hoverClipIndex >= 0 &&
        m_hoverClipIndex < (int)m_timelineClips.size())
    {
        const TimelineClip& hc = m_timelineClips[m_hoverClipIndex];
        double rel = max(0.0, min(1.0,
            (m_hoverTimePosition - hc.clipStartSec) / max(0.001, hc.clipLengthSec)));
        int hx1 = SecondsToTimelineX(hc.startTimeOnTimeline);
        int hx2 = SecondsToTimelineX(hc.startTimeOnTimeline + hc.clipLengthSec);
        int sx = hx1 + (int)((hx2 - hx1) * rel);
        CPen sp(PS_SOLID, 2, RGB(255, 220, 0)); CPen* osp = pDC->SelectObject(&sp);
        pDC->MoveTo(sx, trackTop - 6); pDC->LineTo(sx, trackBottom + 6);
        pDC->SelectObject(osp);
    }

    // Playhead
    if (m_activeTimelineClipIndex >= 0 &&
        m_activeTimelineClipIndex < (int)m_timelineClips.size())
    {
        const TimelineClip& ac = m_timelineClips[m_activeTimelineClipIndex];
        double phTime = ac.startTimeOnTimeline + (m_previewTimePosition - ac.clipStartSec);
        phTime = max(ac.startTimeOnTimeline,
            min(phTime, ac.startTimeOnTimeline + ac.clipLengthSec));
        int phx = SecondsToTimelineX(phTime);

        CPen pp(PS_SOLID, 2, RGB(255, 50, 50)); CPen* opp = pDC->SelectObject(&pp);
        pDC->MoveTo(phx, trackTop - 10); pDC->LineTo(phx, r.bottom - 5);
        POINT tri[3] = { {phx,trackTop - 10},{phx - 6,trackTop - 18},{phx + 6,trackTop - 18} };
        CBrush pb(RGB(255, 50, 50)); CBrush* ob = pDC->SelectObject(&pb);
        pDC->Polygon(tri, 3);
        pDC->SelectObject(opp); pDC->SelectObject(ob);
    }
}

// ─────────────────────────────────────────────────────────────
// Utility helpers
// ─────────────────────────────────────────────────────────────

double NWPVideoEditorView::TimelineXToSeconds(int x) const
{
    int w = m_rcTimeline.Width();
    if (w <= 0) return 0.0;
    return ((double)(x - m_rcTimeline.left) / w) * m_timelineDurationSec
        + m_timelineScrollOffset;
}

int NWPVideoEditorView::SecondsToTimelineX(double seconds) const
{
    if (m_timelineDurationSec <= 0.0) return m_rcTimeline.left;
    int w = m_rcTimeline.Width();
    return m_rcTimeline.left + (int)(
        (seconds - m_timelineScrollOffset) / m_timelineDurationSec * w);
}

BOOL NWPVideoEditorView::IsOverTimeline(CPoint screenPt)
{
    CPoint cp = screenPt; ScreenToClient(&cp);
    return m_rcTimeline.PtInRect(cp);
}

double NWPVideoEditorView::GetCurrentClipDuration()
{
    if (m_activeTimelineClipIndex >= 0 &&
        m_activeTimelineClipIndex < (int)m_timelineClips.size())
        return m_timelineClips[m_activeTimelineClipIndex].clipLengthSec;
    return 0.0;
}

double NWPVideoEditorView::GetVideoDurationAndSize(const CString& filePath, int& outW, int& outH)
{
    outW = 480; outH = 360;
    if (!m_config.IsFFmpegConfigured()) return 30.0;

    // Helper lambda: runs a command, captures stdout, returns output string
    auto RunAndCapture = [](const CString& cmd) -> CString {
        SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
        HANDLE hReadPipe = nullptr, hWritePipe = nullptr;
        if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) return L"";

        // Don't inherit the read end
        SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOW si = { sizeof(si) };
        si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        si.hStdOutput = hWritePipe;
        si.hStdError = hWritePipe;

        PROCESS_INFORMATION pi = {};
        CString mutable_cmd = cmd;
        LPWSTR psz = mutable_cmd.GetBuffer(mutable_cmd.GetLength() + 16);
        BOOL ok = CreateProcessW(nullptr, psz, nullptr, nullptr,
            TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
        mutable_cmd.ReleaseBuffer();

        // Close write end in parent so ReadFile returns when process exits
        CloseHandle(hWritePipe);

        CString result;
        if (ok) {
            char buf[512];
            DWORD bytesRead = 0;
            while (ReadFile(hReadPipe, buf, sizeof(buf) - 1, &bytesRead, nullptr) && bytesRead > 0) {
                buf[bytesRead] = '\0';
                result += CString(buf);
            }
            WaitForSingleObject(pi.hProcess, 10000);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        CloseHandle(hReadPipe);
        return result;
        };

    CString ffprobe = m_config.GetFFprobeExePath().c_str();

    // Get duration — outputs bare number e.g. "120.480000"
    CString durCmd;
    durCmd.Format(
        L"\"%s\" -v error -show_entries format=duration "
        L"-of default=noprint_wrappers=1:nokey=1 \"%s\"",
        ffprobe.GetString(), filePath.GetString());

    // Get width and height — outputs two lines: e.g. "1920\n1080"
    CString sizeCmd;
    sizeCmd.Format(
        L"\"%s\" -v error -select_streams v:0 "
        L"-show_entries stream=width,height "
        L"-of default=noprint_wrappers=1:nokey=1 \"%s\"",
        ffprobe.GetString(), filePath.GetString());

    // Parse duration
    double duration = 30.0;
    CString durOut = RunAndCapture(durCmd);
    durOut.Trim();
    if (!durOut.IsEmpty()) {
        double d = _wtof(durOut);
        if (d > 0.0) duration = d;
    }

    // Parse width/height
    CString sizeOut = RunAndCapture(sizeCmd);
    if (!sizeOut.IsEmpty()) {
        int lineIdx = 0;
        int pos = 0;
        CString token;
        // Split by \n or \r\n
        sizeOut.Replace(L"\r\n", L"\n");
        sizeOut.Replace(L"\r", L"\n");
        while (AfxExtractSubString(token, sizeOut, lineIdx, L'\n')) {
            token.Trim();
            if (!token.IsEmpty()) {
                int val = _wtoi(token);
                if (val > 0) {
                    if (lineIdx == 0) outW = val;
                    if (lineIdx == 1) outH = val;
                }
            }
            lineIdx++;
            if (lineIdx > 4) break;
        }
    }

    return duration;
}

void NWPVideoEditorView::LoadBitmapFromFile(const CString& path)
{
    m_hPreviewBitmap = (HBITMAP)LoadImageW(NULL, path, IMAGE_BITMAP, 0, 0,
        LR_LOADFROMFILE | LR_CREATEDIBSECTION);
}

HBITMAP NWPVideoEditorView::ExtractThumbnail(const CString& videoPath, double timePosition)
{
    if (!m_config.IsFFmpegConfigured()) return nullptr;
    wchar_t tmp[MAX_PATH]; GetTempPath(MAX_PATH, tmp);
    CString tp; tp.Format(L"%sthumb_%d.bmp", tmp, GetTickCount());
    CString cmd;
    cmd.Format(L"\"%s\" -y -ss %.2f -i \"%s\" -vframes 1 -vf scale=160:90 \"%s\"",
        m_config.GetFFmpegExePath().c_str(), timePosition,
        videoPath.GetString(), tp.GetString());
    DWORD ex = 0;
    if (!RunProcessAndWait(cmd, 10000, &ex) || ex != 0)
    {
        DeleteFile(tp); return nullptr;
    }
    if (GetFileAttributes(tp) == INVALID_FILE_ATTRIBUTES) return nullptr;
    HBITMAP bmp = (HBITMAP)LoadImage(NULL, tp, IMAGE_BITMAP, 0, 0,
        LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    DeleteFile(tp);
    return bmp;
}

void NWPVideoEditorView::CreateTestFrame(const CString& filePath, double timePosition)
{
    CClientDC dc(this);
    CDC mdc; mdc.CreateCompatibleDC(&dc);
    m_hPreviewBitmap = CreateCompatibleBitmap(dc.GetSafeHdc(), 480, 360);
    HBITMAP ob = (HBITMAP)mdc.SelectObject(m_hPreviewBitmap);
    mdc.FillSolidRect(0, 0, 480, 360, RGB(40, 40, 60));

    CPen rp(PS_SOLID, 1, RGB(255, 100, 100)); CPen* op = mdc.SelectObject(&rp);
    for (int i = 0;i < 480;i += 40) { mdc.MoveTo(i, 0);mdc.LineTo(i, 360); }
    for (int j = 0;j < 360;j += 30) { mdc.MoveTo(0, j);mdc.LineTo(480, j); }
    mdc.SelectObject(op);

    mdc.SetTextColor(RGB(255, 255, 255)); mdc.SetBkMode(TRANSPARENT);
    CString name = filePath;
    int sl = name.ReverseFind(L'\\'); if (sl != -1) name = name.Mid(sl + 1);
    CRect tr(20, 120, 460, 160); mdc.DrawText(name, &tr, DT_CENTER | DT_WORDBREAK);

    CString ts; ts.Format(L"Time: %.2fs", timePosition);
    CRect tsr(20, 170, 460, 200); mdc.DrawText(ts, &tsr, DT_CENTER | DT_SINGLELINE);

    CString status = m_config.IsFFmpegConfigured()
        ? L"Frame extraction failed" : L"FFmpeg not configured\nClick Settings > Load FFmpeg";
    mdc.SetTextColor(m_config.IsFFmpegConfigured() ? RGB(255, 150, 150) : RGB(255, 200, 100));
    CRect sr(20, 220, 460, 280); mdc.DrawText(status, &sr, DT_CENTER | DT_WORDBREAK);
    mdc.SelectObject(ob);
}

void NWPVideoEditorView::CreateBlackFrame()
{
    CClientDC dc(this);
    CDC mdc; mdc.CreateCompatibleDC(&dc);
    m_hPreviewBitmap = CreateCompatibleBitmap(dc.GetSafeHdc(), 480, 360);
    HBITMAP ob = (HBITMAP)mdc.SelectObject(m_hPreviewBitmap);
    mdc.FillSolidRect(0, 0, 480, 360, RGB(0, 0, 0));
    mdc.SelectObject(ob);
}

void NWPVideoEditorView::ClearPreview()
{
    StopPlayback();
    if (m_hPreviewBitmap) { DeleteObject(m_hPreviewBitmap); m_hPreviewBitmap = nullptr; }
    m_currentPreviewPath.Empty();
    m_previewTimePosition = 0.0;
    InvalidateRect(&m_rcPreview, FALSE);
}

// ─────────────────────────────────────────────────────────────
// Debug
// ─────────────────────────────────────────────────────────────

#ifdef _DEBUG
void NWPVideoEditorView::AssertValid() const { CView::AssertValid(); }
void NWPVideoEditorView::Dump(CDumpContext& dc) const { CView::Dump(dc); }

CNWPVideoEditorDoc* NWPVideoEditorView::GetDocument() const {
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNWPVideoEditorDoc)));
    return (CNWPVideoEditorDoc*)m_pDocument;
}
#endif