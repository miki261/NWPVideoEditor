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

static inline int iround(double v) {
    return static_cast<int>(v >= 0.0 ? v + 0.5 : v - 0.5);
}

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
    ON_BN_CLICKED(ID_LOAD_FFMPEG_BUTTON, &NWPVideoEditorView::OnLoadFFmpegFolder)  // NEW
    ON_WM_TIMER()
END_MESSAGE_MAP()

NWPVideoEditorView::NWPVideoEditorView() {
    m_hPreviewBitmap = nullptr;
    m_previewTimePosition = 0.0;
}

NWPVideoEditorView::~NWPVideoEditorView() {
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
}

BOOL NWPVideoEditorView::PreCreateWindow(CREATESTRUCT& cs) {
    return CView::PreCreateWindow(cs);
}

int NWPVideoEditorView::OnCreate(LPCREATESTRUCT cs) {
    if (CView::OnCreate(cs) == -1) return -1;

    CRect rc(0, 0, 100, 100);
    DWORD style = WS_CHILD | WS_VISIBLE | LVS_ICON | LVS_AUTOARRANGE | LVS_SINGLESEL;
    if (!m_list.Create(style, rc, this, 1001)) return -1;

    m_list.SetExtendedStyle(m_list.GetExtendedStyle()
        | LVS_EX_ONECLICKACTIVATE | LVS_EX_TWOCLICKACTIVATE);

    if (!m_imgLarge.Create(96, 96, ILC_COLOR32 | ILC_MASK, 1, 32)) return -1;
    m_list.SetImageList(&m_imgLarge, LVSIL_NORMAL);

    m_rcPreview = CRect(0, 0, 720, 405);

    // Load string resources for button text
    CString strText;
    strText.LoadString(IDS_BTN_PLAY);
    if (!m_playPauseButton.Create(strText, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        CRect(0, 0, 60, 25), this, ID_PLAY_PAUSE_BUTTON))
        return -1;

    strText.LoadString(IDS_BTN_STOP);
    if (!m_stopButton.Create(strText, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        CRect(0, 0, 60, 25), this, ID_STOP_BUTTON))
        return -1;

    strText.LoadString(IDS_BTN_ADD_TEXT);
    if (!m_addTextButton.Create(strText, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        CRect(0, 0, 70, 25), this, ID_ADD_TEXT_BUTTON))
        return -1;
    strText.LoadString(IDS_BTN_LOAD_FFMPEG);
    if (!m_loadFFmpegButton.Create(strText, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        CRect(0, 0, 100, 25), this, ID_LOAD_FFMPEG_BUTTON))
        return -1;

    return 0;
}

void NWPVideoEditorView::OnDraw(CDC* pDC) {
    DrawTimeline(pDC);
    DrawPreviewFrame(pDC);
}

void NWPVideoEditorView::OnSize(UINT nType, int cx, int cy) {
    CView::OnSize(nType, cx, cy);
    Layout(cx, cy);
}

void NWPVideoEditorView::Layout(int cx, int cy) {
    const int timelineH = 180;
    const int previewW = max(720, cx / 2);
    const int previewH = static_cast<int>(previewW * 9.0 / 16.0);
    const int margin = 10;
    const int buttonH = 25;
    const int buttonW = 60;
    const int addTextButtonW = 70;
    const int loadFFmpegButtonW = 100;
    const int buttonSpacing = 10; 

    if (m_list.GetSafeHwnd()) {
        int listWidth = cx - previewW - margin * 3;
        m_list.MoveWindow(margin, margin, max(120, listWidth), cy - timelineH - margin * 2);
    }

    int previewX = cx - previewW - margin;
    int previewY = margin;
    m_rcPreview = CRect(previewX, previewY, previewX + previewW, previewY + previewH);

    if (m_playPauseButton.GetSafeHwnd()) {
        int buttonY = previewY + previewH + 5;
        m_playPauseButton.MoveWindow(previewX, buttonY, buttonW, buttonH);
        m_stopButton.MoveWindow(previewX + buttonW + buttonSpacing, buttonY, buttonW, buttonH);
        m_addTextButton.MoveWindow(previewX + (buttonW + buttonSpacing) * 2, buttonY, addTextButtonW, buttonH);

        int loadFFmpegX = previewX + previewW - loadFFmpegButtonW;
        m_loadFFmpegButton.MoveWindow(loadFFmpegX, buttonY, loadFFmpegButtonW, buttonH);
    }

    m_rcTimeline = CRect(margin, cy - timelineH, cx - margin, cy - margin);
    Invalidate(FALSE);
}

void NWPVideoEditorView::OnLoadFFmpegFolder()
{
    BROWSEINFO bi = { 0 };
    bi.hwndOwner = GetSafeHwnd();
    CString strText;
    strText.LoadString(IDS_BTN_LOAD_FFMPEG);
    bi.lpszTitle = strText;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl != nullptr) {
        wchar_t folderPath[MAX_PATH];
        if (SHGetPathFromIDList(pidl, folderPath)) {
            CString ffmpegPath, ffprobePath;
            ffmpegPath.Format(L"%s\\ffmpeg.exe", folderPath);
            ffprobePath.Format(L"%s\\ffprobe.exe", folderPath);

            if (PathFileExistsW(ffmpegPath) && PathFileExistsW(ffprobePath)) {
                CString message;
                message.Format(L"✓ FFmpeg found!\n\nFolder: %s\n\nFound:\n• %s\n• %s",
                    folderPath, ffmpegPath.GetString(), ffprobePath.GetString());
                AfxMessageBox(message, MB_OK | MB_ICONINFORMATION);
            }
            else {
                CString message;
                message.Format(L"❌ FFmpeg not found in:\n%s\n\nPlease select a folder containing ffmpeg.exe and ffprobe.exe",
                    folderPath);
                AfxMessageBox(message, MB_OK | MB_ICONWARNING);
            }
        }
        CoTaskMemFree(pidl);
    }
}







int NWPVideoEditorView::AddShellIconForFile(const CString& path) {
    SHFILEINFO sfi;
    if (SHGetFileInfo(path, FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi),
        SHGFI_ICON | SHGFI_LARGEICON) == 0) {
        return -1;
    }

    int idx = m_imgLarge.Add(sfi.hIcon);
    if (sfi.hIcon) DestroyIcon(sfi.hIcon);
    return idx;
}

void NWPVideoEditorView::OnFileImport() {
    const int MAX_FILES = 100;
    const int BUFFER_SIZE = MAX_FILES * MAX_PATH + 1 + 1;
    CString fileBuffer;
    wchar_t* pBuffer = fileBuffer.GetBuffer(BUFFER_SIZE);
    memset(pBuffer, 0, BUFFER_SIZE * sizeof(wchar_t));

    CString filterStr;
    filterStr.LoadString(IDS_FILE_FILTER_MEDIA);
    CFileDialog dlg(TRUE, nullptr, nullptr,
        OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER, filterStr);
    dlg.m_ofn.lpstrFile = pBuffer;
    dlg.m_ofn.nMaxFile = BUFFER_SIZE;

    if (dlg.DoModal() != IDOK) {
        fileBuffer.ReleaseBuffer();
        return;
    }
    fileBuffer.ReleaseBuffer();

    POSITION pos = dlg.GetStartPosition();
    std::vector<CString> selectedFiles;
    while (pos != NULL) {
        CString filePath = dlg.GetNextPathName(pos);
        selectedFiles.push_back(filePath);
    }

    for (size_t i = 0; i < selectedFiles.size(); i++) {
        const CString& filePath = selectedFiles[i];
        ClipItem ci;
        ci.path = filePath;
        ci.iImage = AddShellIconForFile(ci.path);
        ci.durationSec = GetVideoDurationAndSize(ci.path, m_videoWidth, m_videoHeight);
        m_importedClips.push_back(ci);

        CString name = filePath;
        int lastSlash = name.ReverseFind(L'\\');
        if (lastSlash != -1) name = name.Mid(lastSlash + 1);

        int img = ci.iImage >= 0 ? ci.iImage : 0;
        int row = m_list.InsertItem(m_list.GetItemCount(), name, img);

        if (i + 1 == selectedFiles.size()) {
            m_list.SetItemState(row, LVIS_SELECTED, LVIS_SELECTED);
        }
    }

    UpdatePreview();

    if (selectedFiles.size() > 1) {
        CString message;
        message.LoadString(IDS_MSG_IMPORT_SUCCESS);
        CString formatted;
        formatted.Format(message, static_cast<int>(selectedFiles.size()));
        AfxMessageBox(formatted);
    }
}



void NWPVideoEditorView::OnFileSave() {
    OnFileExport();
}

void NWPVideoEditorView::OnFileExport() {
    if (m_timelineClips.empty()) {
        CString message;
        message.LoadString(IDS_MSG_ADD_CLIPS_FIRST);
        AfxMessageBox(message);
        return;
    }

    CFileDialog sfd(FALSE, L"mp4", L"output.mp4", OFN_OVERWRITEPROMPT,
        L"MP4 video|*.mp4|All Files|*.*||");
    sfd.m_ofn.lpstrDefExt = L"mp4";

    if (sfd.DoModal() != IDOK) return;

    CString out = sfd.GetPathName();
    if (out.ReverseFind(L'.') <= 0) out += L".mp4";

    // Sort clips by timeline position
    std::vector<TimelineClip> sortedClips = m_timelineClips;
    std::sort(sortedClips.begin(), sortedClips.end(),
        [](const TimelineClip& a, const TimelineClip& b) {
            return a.startTimeOnTimeline < b.startTimeOnTimeline;
        });

    CString textFilter = BuildTextOverlayFilter();

    if (sortedClips.size() == 1) {
        const TimelineClip& clip = sortedClips[0];
        CString ff;
        if (!textFilter.IsEmpty()) {
            ff.Format(L"cmd /C ffmpeg -y -ss %.3f -i \"%s\" -t %.3f -vf %s -c:v libx264 -preset veryfast -crf 20 -c:a aac \"%s\"",
                clip.clipStartSec, clip.path.GetString(), clip.clipLengthSec, textFilter.GetString(), out.GetString());
        }
        else {
            ff.Format(L"cmd /C ffmpeg -y -ss %.3f -i \"%s\" -t %.3f -c:v libx264 -preset veryfast -crf 20 -c:a aac \"%s\"",
                clip.clipStartSec, clip.path.GetString(), clip.clipLengthSec, out.GetString());
        }
        ExecuteFFmpegCommand(ff);
    }
    else {
        // Multi-clip concatenation with filter_complex
        CString ff(L"cmd /C ffmpeg -y");

        for (size_t i = 0; i < sortedClips.size(); i++) {
            CString part;
            part.Format(L" -ss %.3f -t %.3f -i \"%s\"",
                sortedClips[i].clipStartSec, sortedClips[i].clipLengthSec, sortedClips[i].path.GetString());
            ff += part;
        }

        CString filterComplex = L" -filter_complex \"";
        for (int i = 0; i < static_cast<int>(sortedClips.size()); i++) {
            CString seg;
            seg.Format(L"[%d:v][%d:a]", i, i);
            filterComplex += seg;
        }

        CString concat;
        concat.Format(L"concat=n=%d:v=1:a=1[concatv][concata]", static_cast<int>(sortedClips.size()));
        filterComplex += concat;

        if (!textFilter.IsEmpty()) {
            filterComplex += L";[concatv]";
            filterComplex += textFilter;
            filterComplex += L"[outv]";
            ff += filterComplex;
            ff += L"\" -map [outv] -map [concata]";
        }
        else {
            filterComplex += L"\"";
            ff += filterComplex;
            ff += L" -map [concatv] -map [concata]";
        }

        ff += L" -c:v libx264 -preset veryfast -crf 20 -c:a aac \"";
        ff += out;
        ff += L"\"";

        ExecuteFFmpegCommand(ff);
    }
}

void NWPVideoEditorView::ExecuteFFmpegCommand(const CString& command) {
    CString run = command;
    DWORD exitCode = 0;

    if (!RunProcessAndWait(run, INFINITE, &exitCode)) {
        CString message;
        message.LoadString(IDS_MSG_FFMPEG_NOT_FOUND);
        AfxMessageBox(message);
        return;
    }

    if (exitCode == 0) {
        CString message;
        message.LoadString(IDS_MSG_EXPORT_SUCCESS);
        AfxMessageBox(message);
    }
    else {
        CString message;
        message.LoadString(IDS_MSG_EXPORT_FAILED);
        AfxMessageBox(message);
    }
}



// Static function for process execution
BOOL NWPVideoEditorView::RunProcessAndWait(const CString& cmdLine, DWORD waitMS, DWORD* pExit) {
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};

    CString mutableCmd = cmdLine;
    LPWSTR psz = mutableCmd.GetBuffer(mutableCmd.GetLength() + 16);

    BOOL ok = CreateProcess(nullptr, psz, nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
        nullptr, nullptr, &si, &pi);

    mutableCmd.ReleaseBuffer();

    if (!ok) return FALSE;

    WaitForSingleObject(pi.hProcess, waitMS);
    if (pExit) GetExitCodeProcess(pi.hProcess, pExit);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return TRUE;
}





void NWPVideoEditorView::StartPlayback() {
    if (m_activeTimelineClipIndex >= 0 && m_activeTimelineClipIndex < static_cast<int>(m_timelineClips.size())) {
        m_isPlaying = true;
        m_playbackStartTime = m_previewTimePosition;
        m_playbackStartTick = GetTickCount();
        m_playbackTimer = SetTimer(1, 16, NULL);

        CString strText;
        strText.LoadString(IDS_BTN_PAUSE);
        m_playPauseButton.SetWindowText(strText);
    }
}

void NWPVideoEditorView::StopPlayback() {
    if (m_playbackTimer) {
        KillTimer(m_playbackTimer);
        m_playbackTimer = 0;
    }
    m_isPlaying = false;

    CString strText;
    strText.LoadString(IDS_BTN_PLAY);
    m_playPauseButton.SetWindowText(strText);
}

void NWPVideoEditorView::OnTimer(UINT_PTR nIDEvent) {
    if (nIDEvent == 1 && m_isPlaying) {
        UpdatePlaybackFrame();
    }
    CView::OnTimer(nIDEvent);
}

void NWPVideoEditorView::UpdatePlaybackFrame() {
    if (m_activeTimelineClipIndex < 0 || m_activeTimelineClipIndex >= static_cast<int>(m_timelineClips.size())) {
        StopPlayback();
        return;
    }

    const TimelineClip& activeClip = m_timelineClips[m_activeTimelineClipIndex];
    DWORD currentTick = GetTickCount();
    double elapsed = static_cast<double>(currentTick - m_playbackStartTick) / 1000.0;
    double newTime = m_playbackStartTime + elapsed;
    double clipEndTime = activeClip.clipStartSec + activeClip.clipLengthSec;

    const double eps = 0.0005;
    if (newTime >= clipEndTime - eps) {
        newTime = clipEndTime - 0.016;
        StopPlayback();
    }

    if (fabs(newTime - m_previewTimePosition) > 0.01) {
        m_previewTimePosition = newTime;
        LoadPreviewFrame(activeClip.path, m_previewTimePosition);
    }
}

void NWPVideoEditorView::OnAddText() {
    CTextInputDialog dlg;
    if (dlg.DoModal(this) == IDOK) {
        TextOverlay overlay;
        overlay.text = dlg.GetText();
        overlay.startSec = 0.0;
        overlay.durSec = 5.0;
        overlay.position = CPoint(50, 50);

        m_textOverlays.push_back(overlay);
        m_activeTextOverlayIndex = static_cast<int>(m_textOverlays.size()) - 1;

        InvalidateRect(&m_rcTimeline, FALSE);
        InvalidateRect(&m_rcPreview, FALSE);

        CString message;
        message.LoadString(IDS_MSG_TEXT_ADDED);
        CString formatted;
        formatted.Format(message, overlay.text.GetString());
        AfxMessageBox(formatted);
    }
}

void NWPVideoEditorView::OnEditAddText() {
    OnAddText();
}

void NWPVideoEditorView::OnPlayPause() {
    if (m_isPlaying) {
        StopPlayback();
    }
    else {
        StartPlayback();
    }
}

void NWPVideoEditorView::OnStop() {
    StopPlayback();
    if (m_activeTimelineClipIndex >= 0 && m_activeTimelineClipIndex < static_cast<int>(m_timelineClips.size())) {
        const TimelineClip& activeClip = m_timelineClips[m_activeTimelineClipIndex];
        m_previewTimePosition = activeClip.clipStartSec;
        LoadPreviewFrame(activeClip.path, m_previewTimePosition);
    }
}

int NWPVideoEditorView::HitTestTimelineTextOverlay(CPoint pt) const {
    if (!m_rcTimeline.PtInRect(pt)) return -1;

    const CRect r = m_rcTimeline;
    int textTrackTop = r.bottom - 50;
    int textTrackBottom = r.bottom - 10;

    if (pt.y < textTrackTop || pt.y > textTrackBottom) return -1;

    for (int i = 0; i < static_cast<int>(m_textOverlays.size()); i++) {
        const TextOverlay& overlay = m_textOverlays[i];
        int startX = SecondsToTimelineX(overlay.startSec);
        int endX = SecondsToTimelineX(overlay.startSec + overlay.durSec);
        if (pt.x >= startX && pt.x <= endX) {
            return i;
        }
    }
    return -1;
}

int NWPVideoEditorView::HitTestTextOverlayHandles(CPoint pt) const {
    if (m_activeTextOverlayIndex < 0 || m_activeTextOverlayIndex >= static_cast<int>(m_textOverlays.size()))
        return 0;

    const TextOverlay& overlay = m_textOverlays[m_activeTextOverlayIndex];
    const CRect r = m_rcTimeline;
    int textTrackTop = r.bottom - 50;
    int textTrackBottom = r.bottom - 10;

    int startX = SecondsToTimelineX(overlay.startSec);
    int endX = SecondsToTimelineX(overlay.startSec + overlay.durSec);

    if (pt.x >= startX && pt.x <= startX + 8 && pt.y >= textTrackTop && pt.y <= textTrackBottom) {
        return 1;
    }
    if (pt.x >= endX - 8 && pt.x <= endX && pt.y >= textTrackTop && pt.y <= textTrackBottom) {
        return 2;
    }
    return 0;
}

int NWPVideoEditorView::HitTestTextOverlayInPreview(CPoint pt) const {
    if (!m_rcPreview.PtInRect(pt)) return -1;

    CRect previewInner = m_rcPreview;
    previewInner.DeflateRect(2, 2);
    CPoint relativePt(pt.x - previewInner.left, pt.y - previewInner.top);

    for (int i = static_cast<int>(m_textOverlays.size()) - 1; i >= 0; i--) {
        const TextOverlay& overlay = m_textOverlays[i];
        CRect textBounds(overlay.position.x - 10, overlay.position.y - 10,
            overlay.position.x + 200, overlay.position.y + 40);
        if (textBounds.PtInRect(relativePt)) {
            return i;
        }
    }
    return -1;
}

void NWPVideoEditorView::DrawTextOverlays(CDC* pDC) {
    CRect previewInner = m_rcPreview;
    previewInner.DeflateRect(2, 2);

    for (int i = 0; i < static_cast<int>(m_textOverlays.size()); i++) {
        const TextOverlay& overlay = m_textOverlays[i];

        // Check if text should be visible at current time
        if (m_previewTimePosition < overlay.startSec ||
            m_previewTimePosition > overlay.startSec + overlay.durSec) {
            continue;
        }

        CFont font;
        font.CreateFont(overlay.fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, 0, ANSI_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_SWISS, overlay.fontName);

        CFont* oldFont = pDC->SelectObject(&font);
        pDC->SetBkMode(TRANSPARENT);

        CPoint textPos(previewInner.left + overlay.position.x, previewInner.top + overlay.position.y);

        // Drop shadow
        pDC->SetTextColor(RGB(0, 0, 0));
        pDC->TextOut(textPos.x + 2, textPos.y + 2, overlay.text);

        // Main text
        pDC->SetTextColor(overlay.color);
        pDC->TextOut(textPos.x, textPos.y, overlay.text);

        pDC->SelectObject(oldFont);
    }
}


CString NWPVideoEditorView::BuildTextOverlayFilter() const {
    if (m_textOverlays.empty()) return L"";

    CString filter;
    for (int i = 0; i < static_cast<int>(m_textOverlays.size()); i++) {
        const TextOverlay& overlay = m_textOverlays[i];

        CString textA = overlay.text;
        textA.Replace(L"'", L"\\'");
        textA.Replace(L":", L"\\:");
        textA.Replace(L"\"", L"\\\"");

        // Calculate video coordinates from preview coordinates
        int videoX = iround((overlay.position.x - m_prevOffX) / (m_prevScaleX > 0.0 ? m_prevScaleX : 1.0));
        int videoY = iround((overlay.position.y - m_prevOffY) / (m_prevScaleY > 0.0 ? m_prevScaleY : 1.0));

        if (videoX < 0) videoX = 0;
        if (videoY < 0) videoY = 0;
        if (videoX > m_videoWidth - 1) videoX = m_videoWidth - 1;
        if (videoY > m_videoHeight - 1) videoY = m_videoHeight - 1;

        CString overlayFilter;
        overlayFilter.Format(L"drawtext=text='%s':fontsize=%d:fontcolor=white:x=%d:y=%d:enable='between(t,%.2f,%.2f)'",
            textA.GetString(), overlay.fontSize, videoX, videoY,
            overlay.startSec, overlay.startSec + overlay.durSec);

        if (i == 0) {
            filter = overlayFilter;
        }
        else {
            filter += L"," + overlayFilter;
        }
    }
    return filter;
}


void NWPVideoEditorView::OnContextMenu(CWnd*, CPoint point) {
    ScreenToClient(&point);

    int textHit = HitTestTimelineTextOverlay(point);
    if (textHit >= 0) {
        m_activeTextOverlayIndex = textHit;

        CMenu menu;
        menu.CreatePopupMenu();

        CString menuText;
        menuText.LoadString(IDS_MENU_DELETE);
        CString formatted;
        formatted.Format(menuText, m_textOverlays[textHit].text.GetString());
        menu.AppendMenu(MF_STRING, 32775, formatted);

        CPoint screenPt = point;
        ClientToScreen(&screenPt);
        menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, screenPt.x, screenPt.y, this);
        return;
    }

    if (m_rcTimeline.PtInRect(point)) {
        OnRButtonDown(0, point);
    }
}

void NWPVideoEditorView::OnDeleteTextOverlay() {
    if (m_activeTextOverlayIndex >= 0 && m_activeTextOverlayIndex < static_cast<int>(m_textOverlays.size())) {
        CString message;
        message.LoadString(IDS_MSG_DELETE_TEXT);
        CString formatted;
        formatted.Format(message, m_textOverlays[m_activeTextOverlayIndex].text.GetString());

        if (AfxMessageBox(formatted, MB_YESNO | MB_ICONQUESTION) == IDYES) {
            m_textOverlays.erase(m_textOverlays.begin() + m_activeTextOverlayIndex);
            m_activeTextOverlayIndex = -1;
            InvalidateRect(&m_rcTimeline, FALSE);
            InvalidateRect(&m_rcPreview, FALSE);

            CString deletedMsg;
            deletedMsg.LoadString(IDS_MSG_TEXT_DELETED);
            AfxMessageBox(deletedMsg);
        }
    }
}

// MOUSE HANDLING METHODS
void NWPVideoEditorView::OnLButtonDown(UINT nFlags, CPoint pt) {
    // Text overlay preview dragging
    int previewTextHit = HitTestTextOverlayInPreview(pt);
    if (previewTextHit >= 0) {
        m_draggingTextIndex = previewTextHit;
        m_dragState = DRAG_TEXT_PREVIEW;
        m_textDragStart = pt;
        SetCapture();
        InvalidateRect(&m_rcPreview, FALSE);
        return;
    }

    // Timeline text overlay handling
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

    if (!m_rcTimeline.PtInRect(pt)) {
        CView::OnLButtonDown(nFlags, pt);
        return;
    }

    m_dragState = DRAG_NONE;

    int hitClipIndex = HitTestTimelineClip(pt);
    if (hitClipIndex >= 0) {
        m_activeTimelineClipIndex = hitClipIndex;
        UpdatePreview();

        if (m_hoverTimePosition >= 0.0 && m_hoverClipIndex == hitClipIndex) {
            m_previewTimePosition = m_hoverTimePosition;
            const TimelineClip& clip = m_timelineClips[hitClipIndex];
            LoadPreviewFrame(clip.path, m_previewTimePosition);
        }

        int hit = HitTestTimelineHandle(pt);
        if (hit == 3) {
            m_dragState = DRAG_LEFT_HANDLE;
            m_dragStart = pt;
            const TimelineClip& clip = m_timelineClips[m_activeTimelineClipIndex];
            m_dragStartClipStart = clip.clipStartSec;
            m_dragStartClipLength = clip.clipLengthSec;
            m_dragStartTimelinePos = clip.startTimeOnTimeline;
            SetCapture();
        }
        else if (hit == 2) {
            m_dragState = DRAG_RIGHT_HANDLE;
            m_dragStart = pt;
            const TimelineClip& clip = m_timelineClips[m_activeTimelineClipIndex];
            m_dragStartClipStart = clip.clipStartSec;
            m_dragStartClipLength = clip.clipLengthSec;
            SetCapture();
        }
        else if (hit == 1) {
            m_dragState = DRAG_CLIP_MOVE;
            m_dragStart = pt;
            const TimelineClip& clip = m_timelineClips[m_activeTimelineClipIndex];
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

void NWPVideoEditorView::OnMouseMove(UINT nFlags, CPoint point) {
    // Handle file list dragging
    if (m_bDragging && m_pDragImage) {
        CPoint ptScreen(point);
        ClientToScreen(&ptScreen);
        m_pDragImage->DragMove(ptScreen);

        if (IsOverTimeline(ptScreen)) {
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        }
        else {
            SetCursor(LoadCursor(NULL, IDC_NO));
        }
        return;
    }

    // Handle text preview dragging
    if (m_dragState == DRAG_TEXT_PREVIEW && GetCapture() == this && m_draggingTextIndex >= 0) {
        TextOverlay& overlay = m_textOverlays[m_draggingTextIndex];
        CPoint delta = point - m_textDragStart;

        CRect previewInner = m_rcPreview;
        previewInner.DeflateRect(2, 2);

        CPoint newPos = overlay.position + delta;
        if (newPos.x < 10) newPos.x = 10;
        if (newPos.y < 10) newPos.y = 10;
        if (newPos.x > previewInner.Width() - 200) newPos.x = previewInner.Width() - 200;
        if (newPos.y > previewInner.Height() - 40) newPos.y = previewInner.Height() - 40;

        overlay.position = newPos;
        m_textDragStart = point;
        InvalidateRect(&m_rcPreview, FALSE);
        return;
    }

    // Handle text timeline dragging
    if ((m_dragState == DRAG_TEXT_LEFT_HANDLE || m_dragState == DRAG_TEXT_RIGHT_HANDLE)
        && m_activeTextOverlayIndex >= 0 && GetCapture() == this) {

        TextOverlay& overlay = m_textOverlays[m_activeTextOverlayIndex];
        double deltaTime = TimelineXToSeconds(point.x) - TimelineXToSeconds(m_dragStart.x);

        if (m_dragState == DRAG_TEXT_LEFT_HANDLE) {
            double newStart = max(0.0, m_dragStartTextStart + deltaTime);
            double newDur = max(0.5, m_dragStartTextDur - deltaTime);
            overlay.startSec = newStart;
            overlay.durSec = newDur;
        }
        else if (m_dragState == DRAG_TEXT_RIGHT_HANDLE) {
            overlay.durSec = max(0.5, m_dragStartTextDur + deltaTime);
        }
        InvalidateRect(&m_rcTimeline, FALSE);
        return;
    }

    if (m_dragState == DRAG_TEXT_TIMELINE && GetCapture() == this
        && m_activeTextOverlayIndex >= 0 && m_activeTextOverlayIndex < static_cast<int>(m_textOverlays.size())) {

        TextOverlay& overlay = m_textOverlays[m_activeTextOverlayIndex];
        double deltaTime = TimelineXToSeconds(point.x) - TimelineXToSeconds(m_dragStart.x);
        double newStartTime = max(0.0, m_dragStartTimelinePos + deltaTime);
        overlay.startSec = newStartTime;
        InvalidateRect(&m_rcTimeline, FALSE);
        return;
    }

    // Handle clip dragging
    if (m_dragState != DRAG_NONE && GetCapture() == this
        && m_activeTimelineClipIndex >= 0 && m_activeTimelineClipIndex < static_cast<int>(m_timelineClips.size())) {

        TimelineClip& activeClip = m_timelineClips[m_activeTimelineClipIndex];

        if (m_dragState == DRAG_LEFT_HANDLE) {
            double newTimelinePos = m_dragStartTimelinePos + TimelineXToSeconds(point.x) - TimelineXToSeconds(m_dragStart.x);
            if (newTimelinePos < 0.0) newTimelinePos = 0.0;
            activeClip.startTimeOnTimeline = newTimelinePos;

            double deltaSeconds = TimelineXToSeconds(point.x) - TimelineXToSeconds(m_dragStart.x);
            double newClipStart = max(0.0, min(activeClip.originalDuration - 1.0, m_dragStartClipStart - deltaSeconds));
            double rightEdgeTime = m_dragStartTimelinePos + m_dragStartClipLength;
            double newLength = max(1.0, rightEdgeTime - newTimelinePos);

            if (newClipStart + newLength > activeClip.originalDuration) {
                newLength = activeClip.originalDuration - newClipStart;
            }
            if (newLength < 1.0) {
                newLength = 1.0;
                newClipStart = max(0.0, activeClip.originalDuration - newLength);
            }

            activeClip.startTimeOnTimeline = newTimelinePos;
            activeClip.clipStartSec = newClipStart;
            activeClip.clipLengthSec = newLength;
        }
        else if (m_dragState == DRAG_RIGHT_HANDLE) {
            double currentTime = TimelineXToSeconds(point.x);
            double deltaTime = currentTime - TimelineXToSeconds(m_dragStart.x);
            double newLength = m_dragStartClipLength + deltaTime;
            double maxLength = activeClip.originalDuration - activeClip.clipStartSec;
            activeClip.clipLengthSec = max(1.0, min(maxLength, newLength));
        }
        else if (m_dragState == DRAG_CLIP_MOVE) {
            double currentTime = TimelineXToSeconds(point.x);
            double deltaTime = currentTime - TimelineXToSeconds(m_dragStart.x);
            double newPos = max(0.0, m_dragStartTimelinePos + deltaTime);

            bool canMove = true;
            for (int i = 0; i < static_cast<int>(m_timelineClips.size()); i++) {
                if (i == m_activeTimelineClipIndex) continue;

                const TimelineClip& otherClip = m_timelineClips[i];
                double otherStart = otherClip.startTimeOnTimeline;
                double otherEnd = otherClip.startTimeOnTimeline + otherClip.clipLengthSec;
                double thisEnd = newPos + activeClip.clipLengthSec;

                if (!(thisEnd <= otherStart || newPos >= otherEnd)) {
                    canMove = false;
                    break;
                }
            }

            if (canMove) {
                activeClip.startTimeOnTimeline = newPos;
            }
        }
        InvalidateRect(&m_rcTimeline, FALSE);
        return;
    }

    // Handle hover on timeline
    if (m_rcTimeline.PtInRect(point) && m_dragState == DRAG_NONE) {
        int hitClipIndex = HitTestTimelineClip(point);
        if (hitClipIndex >= 0) {
            m_hoverClipIndex = hitClipIndex;
            const TimelineClip& clip = m_timelineClips[hitClipIndex];

            int clipStartX = SecondsToTimelineX(clip.startTimeOnTimeline);
            int clipEndX = SecondsToTimelineX(clip.startTimeOnTimeline + clip.clipLengthSec);
            double clipRatio = static_cast<double>(point.x - clipStartX) / (clipEndX - clipStartX);
            clipRatio = max(0.0, min(1.0, clipRatio));

            double newHover = clip.clipStartSec + clipRatio * clip.clipLengthSec;
            bool changed = fabs(newHover - m_hoverTimePosition) > 0.02;

            m_hoverTimePosition = newHover;
            m_showSelectionBar = true;

            if (changed && hitClipIndex == m_activeTimelineClipIndex) {
                LoadPreviewFrame(clip.path, m_hoverTimePosition);
            }
            InvalidateRect(&m_rcTimeline, FALSE);
        }
        else {
            if (m_showSelectionBar) {
                m_showSelectionBar = false;
                m_hoverClipIndex = -1;
                m_hoverTimePosition = -1.0;
                InvalidateRect(&m_rcTimeline, FALSE);
            }
        }
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    }
    else {
        if (m_showSelectionBar) {
            m_showSelectionBar = false;
            m_hoverClipIndex = -1;
            m_hoverTimePosition = -1.0;
            InvalidateRect(&m_rcTimeline, FALSE);
        }
    }

    if (m_rcPreview.PtInRect(point) && HitTestTextOverlayInPreview(point) >= 0) {
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));
    }
}

void NWPVideoEditorView::OnLButtonUp(UINT nFlags, CPoint point) {
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

        CPoint ptScreen(point);
        ClientToScreen(&ptScreen);
        if (IsOverTimeline(ptScreen) && m_nDragIndex >= 0 && m_nDragIndex < static_cast<int>(m_importedClips.size())) {
            AddClipToTimeline(m_importedClips[m_nDragIndex].path);
        }
        m_nDragIndex = -1;
        return;
    }

    if (m_dragState != DRAG_NONE) {
        if (GetCapture() == this) ReleaseCapture();
        m_dragState = DRAG_NONE;
        InvalidateRect(&m_rcTimeline, FALSE);
    }

    CView::OnLButtonUp(nFlags, point);
}

void NWPVideoEditorView::OnRButtonDown(UINT nFlags, CPoint point) {
    if (!m_rcTimeline.PtInRect(point)) {
        CView::OnRButtonDown(nFlags, point);
        return;
    }

    int hitClipIndex = HitTestTimelineClip(point);
    if (hitClipIndex >= 0) {
        m_contextMenuClipIndex = hitClipIndex;
        m_activeTimelineClipIndex = hitClipIndex;
        InvalidateRect(&m_rcTimeline, FALSE);

        CMenu contextMenu;
        contextMenu.CreatePopupMenu();

        CString clipName = m_timelineClips[hitClipIndex].path;
        int lastSlash = clipName.ReverseFind(L'\\');
        if (lastSlash != -1) clipName = clipName.Mid(lastSlash + 1);

        CString splitText;
        if (m_showSelectionBar && m_hoverClipIndex == hitClipIndex) {
            splitText.LoadString(IDS_MENU_SPLIT_AT);
            CString formatted;
            formatted.Format(splitText, clipName.GetString(), m_hoverTimePosition);
            splitText = formatted;
        }
        else {
            splitText.LoadString(IDS_MENU_SPLIT);
            CString formatted;
            formatted.Format(splitText, clipName.GetString());
            splitText = formatted;
        }

        contextMenu.AppendMenu(MF_STRING, ID_TIMELINE_SPLIT_CLIP, splitText);
        contextMenu.AppendMenu(MF_SEPARATOR);

        CString removeText;
        removeText.LoadString(IDS_MENU_REMOVE_FROM_TIMELINE);
        CString formatted;
        formatted.Format(removeText, clipName.GetString());
        contextMenu.AppendMenu(MF_STRING, ID_TIMELINE_REMOVE_CLIP, formatted);

        CPoint screenPoint = point;
        ClientToScreen(&screenPoint);
        contextMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, screenPoint.x, screenPoint.y, this);
    }
    else {
        CView::OnRButtonDown(nFlags, point);
    }
}

void NWPVideoEditorView::OnTimelineRemoveClip() {
    if (m_contextMenuClipIndex >= 0 && m_contextMenuClipIndex < static_cast<int>(m_timelineClips.size())) {
        CString clipName = m_timelineClips[m_contextMenuClipIndex].path;
        int lastSlash = clipName.ReverseFind(L'\\');
        if (lastSlash != -1) clipName = clipName.Mid(lastSlash + 1);

        CString message;
        message.LoadString(IDS_MSG_REMOVE_CLIP);
        CString formatted;
        formatted.Format(message, clipName.GetString());

        if (AfxMessageBox(formatted, MB_YESNO | MB_ICONQUESTION) == IDYES) {
            m_timelineClips.erase(m_timelineClips.begin() + m_contextMenuClipIndex);

            if (m_activeTimelineClipIndex == m_contextMenuClipIndex) {
                m_activeTimelineClipIndex = -1;
            }
            else if (m_activeTimelineClipIndex > m_contextMenuClipIndex) {
                m_activeTimelineClipIndex--;
            }

            RepositionClipsAfterRemoval();
            UpdatePreview();
            InvalidateRect(&m_rcTimeline, FALSE);
        }

        m_contextMenuClipIndex = -1;
    }
}

void NWPVideoEditorView::OnTimelineSplitClip() {
    if (m_contextMenuClipIndex < 0 || m_contextMenuClipIndex >= static_cast<int>(m_timelineClips.size())) {
        return;
    }

    TimelineClip originalClip = m_timelineClips[m_contextMenuClipIndex];
    double splitTime;

    if (m_showSelectionBar && m_hoverClipIndex == m_contextMenuClipIndex) {
        splitTime = m_hoverTimePosition;
    }
    else {
        splitTime = originalClip.clipStartSec + originalClip.clipLengthSec / 2.0;
    }

    if (splitTime <= originalClip.clipStartSec || splitTime >= originalClip.clipStartSec + originalClip.clipLengthSec) {
        CString message;
        message.LoadString(IDS_MSG_INVALID_SPLIT);
        AfxMessageBox(message);
        return;
    }

    TimelineClip secondClip = originalClip;
    secondClip.startTimeOnTimeline = originalClip.startTimeOnTimeline + (splitTime - originalClip.clipStartSec);
    secondClip.clipStartSec = splitTime;
    secondClip.clipLengthSec = originalClip.clipStartSec + originalClip.clipLengthSec - splitTime;

    originalClip.clipLengthSec = splitTime - originalClip.clipStartSec;

    m_timelineClips.insert(m_timelineClips.begin() + m_contextMenuClipIndex + 1, secondClip);

    double adjustmentTime = secondClip.clipLengthSec;
    for (int i = m_contextMenuClipIndex + 2; i < static_cast<int>(m_timelineClips.size()); i++) {
        m_timelineClips[i].startTimeOnTimeline += adjustmentTime;
    }

    m_activeTimelineClipIndex = m_contextMenuClipIndex;
    m_showSelectionBar = false;
    m_hoverClipIndex = -1;
    m_hoverTimePosition = -1.0;

    UpdatePreview();
    InvalidateRect(&m_rcTimeline, FALSE);

    m_contextMenuClipIndex = -1;
}

double NWPVideoEditorView::GetCurrentClipDuration() {
    if (m_activeTimelineClipIndex >= 0 && m_activeTimelineClipIndex < static_cast<int>(m_timelineClips.size())) {
        return m_timelineClips[m_activeTimelineClipIndex].clipLengthSec;
    }
    return 0.0;
}

void NWPVideoEditorView::RepositionClipsAfterRemoval() {
    std::sort(m_timelineClips.begin(), m_timelineClips.end(),
        [](const TimelineClip& a, const TimelineClip& b) {
            return a.startTimeOnTimeline < b.startTimeOnTimeline;
        });

    double currentPos = 0.0;
    for (size_t i = 0; i < m_timelineClips.size(); i++) {
        m_timelineClips[i].startTimeOnTimeline = currentPos;
        currentPos += m_timelineClips[i].clipLengthSec;
    }
}

void NWPVideoEditorView::SetActiveClipFromSelection() {
    int sel = m_list.GetNextItem(-1, LVNI_SELECTED);
    if (sel < 0 || sel >= static_cast<int>(m_importedClips.size())) {
        return;
    }

    UpdatePreview();
    InvalidateRect(&m_rcTimeline, FALSE);
}

double NWPVideoEditorView::GetVideoDurationAndSize(const CString& filePath, int& outW, int& outH) {
    outW = 480;
    outH = 360;

    // Use ffprobe to get video dimensions and duration
    wchar_t tmpPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tmpPath);

    CString whOut;
    whOut.Format(L"%sffprobe_wh_%llu.txt", tmpPath, static_cast<unsigned long long>(GetTickCount()));

    CString cmdWH;
    cmdWH.Format(L"cmd /C ffprobe -v error -select_streams v:0 -show_entries stream=width,height -of default=nw=1 \"%s\" > \"%s\" 2>&1",
        filePath.GetString(), whOut.GetString());

    DWORD exitWH = 0;
    RunProcessAndWait(cmdWH, 7000, &exitWH);

    CStdioFile f;
    if (f.Open(whOut, CFile::modeRead | CFile::typeText)) {
        CString line;
        while (f.ReadString(line)) {
            int pW = line.Find(L"width=");
            int pH = line.Find(L"height=");
            if (pW >= 0) outW = _wtoi(line.Mid(pW + 6));
            if (pH >= 0) outH = _wtoi(line.Mid(pH + 7));
        }
        f.Close();
    }
    DeleteFileW(whOut);

    // Get duration
    CString durOut;
    durOut.Format(L"%sffprobe_dur_%llu.txt", tmpPath, static_cast<unsigned long long>(GetTickCount()));

    CString cmdDur;
    cmdDur.Format(L"cmd /C ffprobe -v error -select_streams v:0 -show_entries format=duration -of default=nw=1:nk=1 \"%s\" > \"%s\" 2>&1",
        filePath.GetString(), durOut.GetString());

    DWORD exitDur = 0;
    RunProcessAndWait(cmdDur, 7000, &exitDur);

    double duration = 30.0; // default
    if (f.Open(durOut, CFile::modeRead | CFile::typeText)) {
        CString s;
        if (f.ReadString(s)) {
            double d = _wtof(s);
            if (d > 0) duration = d;
        }
        f.Close();
    }
    DeleteFileW(durOut);

    return duration;
}





void NWPVideoEditorView::AddClipToTimeline(const CString& clipPath) {
    double originalDuration = 30.0;
    int imageIndex = -1;

    for (size_t i = 0; i < m_importedClips.size(); i++) {
        if (m_importedClips[i].path == clipPath) {
            originalDuration = m_importedClips[i].durationSec > 0 ? m_importedClips[i].durationSec : 30.0;
            imageIndex = m_importedClips[i].iImage;
            break;
        }
    }

    double newStartTime = 0.0;
    for (size_t i = 0; i < m_timelineClips.size(); i++) {
        double clipEnd = m_timelineClips[i].startTimeOnTimeline + m_timelineClips[i].clipLengthSec;
        if (clipEnd > newStartTime) {
            newStartTime = clipEnd;
        }
    }

    TimelineClip newClip;
    newClip.path = clipPath;
    newClip.startTimeOnTimeline = newStartTime;
    newClip.clipStartSec = 0.0;
    newClip.clipLengthSec = min(10.0, originalDuration);
    newClip.originalDuration = originalDuration;
    newClip.iImage = imageIndex;

    m_timelineClips.push_back(newClip);
    m_activeTimelineClipIndex = static_cast<int>(m_timelineClips.size()) - 1;

    GetVideoDurationAndSize(clipPath, m_videoWidth, m_videoHeight);

    UpdatePreview();
    m_dragState = DRAG_NONE;
    m_bDragging = FALSE;
    InvalidateRect(&m_rcTimeline, FALSE);
}

void NWPVideoEditorView::OnListDblClk(NMHDR* pNMHDR, LRESULT* pResult) {
    NMITEMACTIVATE* pNMItemActivate = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    int i = pNMItemActivate->iItem;

    if (i >= 0 && i < static_cast<int>(m_importedClips.size())) {
        AddClipToTimeline(m_importedClips[i].path);
    }

    if (pResult) *pResult = 0;
}

void NWPVideoEditorView::OnListItemActivate(NMHDR* pNMHDR, LRESULT* pResult) {
    SetActiveClipFromSelection();
    if (pResult) *pResult = 0;
}

void NWPVideoEditorView::OnListBeginDrag(NMHDR* pNMHDR, LRESULT* pResult) {
    NMLISTVIEW* pNMListView = (NMLISTVIEW*)pNMHDR;
    m_nDragIndex = pNMListView->iItem;

    if (m_nDragIndex >= 0 && m_nDragIndex < static_cast<int>(m_importedClips.size())) {
        POINT pt = { 10, 10 };
        m_pDragImage = m_list.CreateDragImage(m_nDragIndex, &pt);

        if (m_pDragImage) {
            m_pDragImage->BeginDrag(0, CPoint(10, 10));
            m_pDragImage->DragEnter(GetDesktopWindow(), pNMListView->ptAction);
            m_bDragging = TRUE;
            SetCapture();
        }
    }

    *pResult = 0;
}

BOOL NWPVideoEditorView::IsOverTimeline(CPoint screenPt) {
    CPoint clientPt = screenPt;
    ScreenToClient(&clientPt);
    return m_rcTimeline.PtInRect(clientPt);
}

int NWPVideoEditorView::HitTestTimelineClip(CPoint pt) const {
    if (m_timelineClips.empty()) return -1;

    const CRect r = m_rcTimeline;
    int top = r.top + 48;
    int bottom = r.bottom - 56;

    for (int i = 0; i < static_cast<int>(m_timelineClips.size()); i++) {
        const TimelineClip& clip = m_timelineClips[i];
        int clipStartX = SecondsToTimelineX(clip.startTimeOnTimeline);
        int clipEndX = SecondsToTimelineX(clip.startTimeOnTimeline + clip.clipLengthSec);
        CRect clipRect(clipStartX, top, clipEndX, bottom);
        if (clipRect.PtInRect(pt)) {
            return i;
        }
    }
    return -1;
}

int NWPVideoEditorView::HitTestTimelineHandle(CPoint pt) const {
    if (m_activeTimelineClipIndex < 0 || m_activeTimelineClipIndex >= static_cast<int>(m_timelineClips.size())) {
        return 0;
    }

    const TimelineClip& activeClip = m_timelineClips[m_activeTimelineClipIndex];
    const CRect r = m_rcTimeline;
    int top = r.top + 48;
    int bottom = r.bottom - 56;

    int clipStartX = SecondsToTimelineX(activeClip.startTimeOnTimeline);
    int clipEndX = SecondsToTimelineX(activeClip.startTimeOnTimeline + activeClip.clipLengthSec);

    CRect clipR(clipStartX, top, clipEndX, bottom);

    CRect leftHandleR(clipR.left, clipR.top, clipR.left + 8, clipR.bottom);
    if (leftHandleR.PtInRect(pt)) return 3;

    CRect rightHandleR(clipR.right - 8, clipR.top, clipR.right, clipR.bottom);
    if (rightHandleR.PtInRect(pt)) return 2;

    if (clipR.PtInRect(pt)) return 1;

    return 0;
}

void NWPVideoEditorView::UpdatePreview() {
    StopPlayback();

    if (m_activeTimelineClipIndex >= 0 && m_activeTimelineClipIndex < static_cast<int>(m_timelineClips.size())) {
        const TimelineClip& activeClip = m_timelineClips[m_activeTimelineClipIndex];
        GetVideoDurationAndSize(activeClip.path, m_videoWidth, m_videoHeight);
        m_previewTimePosition = activeClip.clipStartSec;
        LoadPreviewFrame(activeClip.path, m_previewTimePosition);
    }
    else {
        int sel = m_list.GetNextItem(-1, LVNI_SELECTED);
        if (sel >= 0 && sel < static_cast<int>(m_importedClips.size())) {
            GetVideoDurationAndSize(m_importedClips[sel].path, m_videoWidth, m_videoHeight);
            m_previewTimePosition = 0.0;
            LoadPreviewFrame(m_importedClips[sel].path, m_previewTimePosition);
        }
        else {
            ClearPreview();
        }
    }
}

void NWPVideoEditorView::DrawTimeline(CDC* pDC) {
    if (m_rcTimeline.IsRectEmpty()) return;

    // Background
    pDC->FillSolidRect(&m_rcTimeline, RGB(32, 32, 32));
    pDC->Draw3dRect(&m_rcTimeline, RGB(80, 80, 80), RGB(40, 40, 40));

    const CRect r = m_rcTimeline;
    int trackTop = r.top + 48;
    int trackBottom = r.bottom - 56;

    // Time grid
    CPen gridPen(PS_SOLID, 1, RGB(60, 60, 60));
    CPen* oldPen = pDC->SelectObject(&gridPen);

    for (int s = 0; s < static_cast<int>(m_timelineDurationSec); s += 5) {
        int x = SecondsToTimelineX(static_cast<double>(s));
        pDC->MoveTo(x, r.top + 8);
        pDC->LineTo(x, r.bottom - 8);
    }
    pDC->SelectObject(oldPen);

    // Clips
    for (int i = 0; i < static_cast<int>(m_timelineClips.size()); i++) {
        const TimelineClip& clip = m_timelineClips[i];
        int x1 = SecondsToTimelineX(clip.startTimeOnTimeline);
        int x2 = SecondsToTimelineX(clip.startTimeOnTimeline + clip.clipLengthSec);

        CRect rc(x1, trackTop, x2, trackBottom);
        COLORREF fill = (i == m_activeTimelineClipIndex) ? RGB(70, 120, 200) : RGB(80, 80, 120);

        pDC->FillSolidRect(&rc, fill);
        pDC->FrameRect(&rc, CBrush::FromHandle((HBRUSH)GetStockObject(BLACK_BRUSH)));

        // Handles
        CRect lh(rc.left, rc.top, rc.left + 8, rc.bottom);
        CRect rh(rc.right - 8, rc.top, rc.right, rc.bottom);
        pDC->FillSolidRect(&lh, RGB(200, 200, 200));
        pDC->FillSolidRect(&rh, RGB(200, 200, 200));

        // Name
        CString name = clip.path;
        int lastSlash = name.ReverseFind(L'\\');
        if (lastSlash != -1) name = name.Mid(lastSlash + 1);

        pDC->SetBkMode(TRANSPARENT);
        pDC->SetTextColor(RGB(230, 230, 230));
        CRect trc = rc;
        trc.DeflateRect(10, 4);
        pDC->DrawText(name, &trc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    // Text overlay track
    CRect textTrack(r.left + 10, r.bottom - 50, r.right - 10, r.bottom - 10);
    pDC->FillSolidRect(&textTrack, RGB(45, 45, 45));
    pDC->FrameRect(&textTrack, CBrush::FromHandle((HBRUSH)GetStockObject(BLACK_BRUSH)));

    for (int i = 0; i < static_cast<int>(m_textOverlays.size()); i++) {
        const TextOverlay& t = m_textOverlays[i];
        int sx = SecondsToTimelineX(t.startSec);
        int ex = SecondsToTimelineX(t.startSec + t.durSec);

        CRect tr(sx, textTrack.top + 4, ex, textTrack.bottom - 4);
        COLORREF tf = (i == m_activeTextOverlayIndex) ? RGB(200, 140, 70) : RGB(150, 110, 60);

        pDC->FillSolidRect(&tr, tf);
        pDC->FrameRect(&tr, CBrush::FromHandle((HBRUSH)GetStockObject(BLACK_BRUSH)));

        // Handles on text item
        CRect tlh(tr.left, tr.top, tr.left + 8, tr.bottom);
        CRect trh(tr.right - 8, tr.top, tr.right, tr.bottom);
        pDC->FillSolidRect(&tlh, RGB(230, 230, 230));
        pDC->FillSolidRect(&trh, RGB(230, 230, 230));

        CString label = t.text;
        pDC->SetBkMode(TRANSPARENT);
        pDC->SetTextColor(RGB(15, 15, 15));
        CRect lrc = tr;
        lrc.DeflateRect(10, 2);
        pDC->DrawText(label, &lrc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    // Hover selection bar
    if (m_showSelectionBar && m_hoverClipIndex >= 0 && m_hoverClipIndex < static_cast<int>(m_timelineClips.size())) {
        const TimelineClip& c = m_timelineClips[m_hoverClipIndex];
        double rel = (m_hoverTimePosition - c.clipStartSec) / max(0.001, c.clipLengthSec);
        rel = max(0.0, min(1.0, rel));

        int cx1 = SecondsToTimelineX(c.startTimeOnTimeline);
        int cx2 = SecondsToTimelineX(c.startTimeOnTimeline + c.clipLengthSec);
        int sx = cx1 + static_cast<int>((cx2 - cx1) * rel);

        CPen selPen(PS_SOLID, 1, RGB(255, 220, 0));
        CPen* oldPen2 = pDC->SelectObject(&selPen);
        pDC->MoveTo(sx, trackTop - 6);
        pDC->LineTo(sx, trackBottom + 6);
        pDC->SelectObject(oldPen2);
    }
}

double NWPVideoEditorView::TimelineXToSeconds(int x) const {

    int left = m_rcTimeline.left + 10;
    int right = m_rcTimeline.right - 10;
    if (right <= left) return 0.0;

    x = max(left, min(x, right));
    double ratio = static_cast<double>(x - left) / static_cast<double>(right - left);
    return ratio * m_timelineDurationSec;
}

int NWPVideoEditorView::SecondsToTimelineX(double seconds) const {
    seconds = max(0.0, min(seconds, m_timelineDurationSec));

    int left = m_rcTimeline.left + 10;
    int right = m_rcTimeline.right - 10;
    if (right <= left) return left;

    double ratio = seconds / m_timelineDurationSec;
    return left + static_cast<int>(ratio * static_cast<double>(right - left));
}

void NWPVideoEditorView::DrawPreviewFrame(CDC* pDC) {
    if (m_rcPreview.IsRectEmpty()) return;

    pDC->FillSolidRect(&m_rcPreview, RGB(20, 20, 20));
    pDC->Draw3dRect(&m_rcPreview, RGB(80, 80, 80), RGB(40, 40, 40));

    CRect innerRect = m_rcPreview;
    innerRect.DeflateRect(2, 2);

    int boxW = innerRect.Width();
    int boxH = innerRect.Height();
    int vidW = max(1, m_videoWidth);
    int vidH = max(1, m_videoHeight);

    double sx = static_cast<double>(boxW) / vidW;
    double sy = static_cast<double>(boxH) / vidH;
    double s = (sx < sy) ? sx : sy;

    int drawW = iround(vidW * s);
    int drawH = iround(vidH * s);

    int dstL = innerRect.left + (boxW - drawW) / 2;
    int dstT = innerRect.top + (boxH - drawH) / 2;

    m_prevScaleX = m_prevScaleY = (s > 0.0) ? s : 1.0;
    m_prevOffX = dstL - innerRect.left;
    m_prevOffY = dstT - innerRect.top;

    if (m_hPreviewBitmap) {
        CDC memDC;
        memDC.CreateCompatibleDC(pDC);
        HBITMAP oldBitmap = (HBITMAP)memDC.SelectObject(m_hPreviewBitmap);

        BITMAP bmp;
        GetObject(m_hPreviewBitmap, sizeof(BITMAP), &bmp);

        // Fill black bars
        pDC->FillSolidRect(innerRect.left, innerRect.top, boxW, m_prevOffY, RGB(20, 20, 20));
        pDC->FillSolidRect(innerRect.left, innerRect.top + m_prevOffY + drawH, boxW, boxH - m_prevOffY - drawH, RGB(20, 20, 20));
        pDC->FillSolidRect(innerRect.left, innerRect.top + m_prevOffY, m_prevOffX, drawH, RGB(20, 20, 20));
        pDC->FillSolidRect(innerRect.left + m_prevOffX + drawW, innerRect.top + m_prevOffY, boxW - m_prevOffX - drawW, drawH, RGB(20, 20, 20));

        pDC->StretchBlt(dstL, dstT, drawW, drawH, &memDC, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
        memDC.SelectObject(oldBitmap);
    }
    else {
        pDC->FillSolidRect(&innerRect, RGB(40, 40, 40));
        pDC->SetTextColor(RGB(150, 150, 150));
        pDC->SetBkMode(TRANSPARENT);

        CString loadingText;
        loadingText.LoadString(IDS_LBL_LOADING_FRAME);
        pDC->DrawText(loadingText, &innerRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    DrawTextOverlays(pDC);

    // Info bar
    CRect infoRect = m_rcPreview;
    infoRect.top = infoRect.bottom + 35;
    infoRect.bottom = infoRect.top + 20;

    if (!m_currentPreviewPath.IsEmpty()) {
        CString filename = m_currentPreviewPath;
        int lastSlash = filename.ReverseFind(L'\\');
        if (lastSlash != -1) filename = filename.Mid(lastSlash + 1);

        CString info;
        CString playingText;
        playingText.LoadString(m_isPlaying ? IDS_LBL_PLAYING : IDS_LBL_LOADING_FRAME);

        CString formatText;
        formatText.LoadString(IDS_LBL_TIME_FORMAT);
        info.Format(formatText, filename.GetString(), m_previewTimePosition, playingText.GetString());

        pDC->SetTextColor(RGB(200, 200, 200));
        pDC->SetBkMode(TRANSPARENT);
        pDC->DrawText(info, &infoRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
}

void NWPVideoEditorView::LoadPreviewFrame(const CString& filePath, double timePosition) {
    m_currentPreviewPath = filePath;

    if (m_hPreviewBitmap) {
        DeleteObject(m_hPreviewBitmap);
        m_hPreviewBitmap = nullptr;
    }

    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);

    CString tempImagePath;
    tempImagePath.Format(L"%stemp_frame_%llu.bmp", tempPath, static_cast<unsigned long long>(GetTickCount()));

    CString ff;
    ff.Format(L"cmd /C ffmpeg -y -ss %.3f -i \"%s\" -vframes 1 "
        L"-vf \"scale=in_color_matrix=bt709:out_color_matrix=bt709\" "
        L"-colorspace bt709 -color_primaries bt709 -color_trc iec61966-2-1 "
        L"-src_range 1 -dst_range 1 \"%s\" > nul 2>&1",
        max(0.0, timePosition), filePath.GetString(), tempImagePath.GetString());

    CString run = ff;
    RunProcessAndWait(run, 8000, NULL);

    if (GetFileAttributesW(tempImagePath) != INVALID_FILE_ATTRIBUTES) {
        LoadBitmapFromFile(tempImagePath);
        DeleteFileW(tempImagePath);
    }

    if (!m_hPreviewBitmap) {
        CreateTestFrame(filePath, timePosition);
    }

    InvalidateRect(&m_rcPreview, FALSE);
}


void NWPVideoEditorView::LoadBitmapFromFile(const CString& imagePath) {
    m_hPreviewBitmap = (HBITMAP)LoadImageW(NULL, imagePath, IMAGE_BITMAP, 0, 0,
        LR_LOADFROMFILE | LR_CREATEDIBSECTION);
}


void NWPVideoEditorView::CreateTestFrame(const CString& filePath, double timePosition) {
    CClientDC dc(this);
    CDC memDC;
    memDC.CreateCompatibleDC(&dc);

    m_hPreviewBitmap = CreateCompatibleBitmap(dc.GetSafeHdc(), 480, 360);
    HBITMAP oldBitmap = (HBITMAP)memDC.SelectObject(m_hPreviewBitmap);

    memDC.FillSolidRect(0, 0, 480, 360, RGB(40, 40, 60));

    CPen redPen(PS_SOLID, 2, RGB(255, 100, 100));
    CPen* oldPen = memDC.SelectObject(&redPen);

    for (int i = 0; i < 480; i += 40) {
        memDC.MoveTo(i, 0);
        memDC.LineTo(i, 360);
    }
    for (int j = 0; j < 360; j += 30) {
        memDC.MoveTo(0, j);
        memDC.LineTo(480, j);
    }

    memDC.SelectObject(oldPen);

    memDC.SetTextColor(RGB(255, 255, 255));
    memDC.SetBkMode(TRANSPARENT);

    CString filename = filePath;
    int lastSlash = filename.ReverseFind(L'\\');
    if (lastSlash != -1) filename = filename.Mid(lastSlash + 1);

    CRect titleRect(20, 120, 460, 160);
    memDC.DrawText(filename, &titleRect, DT_CENTER | DT_VCENTER | DT_WORDBREAK);

    CString timeText;
    timeText.Format(L"Time: %.2fs", timePosition);
    CRect timeRect(20, 170, 460, 200);
    memDC.DrawText(timeText, &timeRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // Show FFmpeg status
    CString statusText;
    memDC.SetTextColor(RGB(255, 200, 100));
    statusText.LoadString(IDS_LBL_FFMPEG_FAILED);


    CRect statusRect(20, 220, 460, 280);
    memDC.DrawText(statusText, &statusRect, DT_CENTER | DT_VCENTER | DT_WORDBREAK);

    memDC.SelectObject(oldBitmap);
}


void NWPVideoEditorView::CreateBlackFrame() {
    CClientDC dc(this);
    CDC memDC;
    memDC.CreateCompatibleDC(&dc);

    m_hPreviewBitmap = CreateCompatibleBitmap(dc.GetSafeHdc(), 480, 360);
    HBITMAP oldBitmap = (HBITMAP)memDC.SelectObject(m_hPreviewBitmap);

    memDC.FillSolidRect(0, 0, 480, 360, RGB(0, 0, 0));

    memDC.SelectObject(oldBitmap);
}

void NWPVideoEditorView::ClearPreview() {
    StopPlayback();

    if (m_hPreviewBitmap) {
        DeleteObject(m_hPreviewBitmap);
        m_hPreviewBitmap = nullptr;
    }

    m_currentPreviewPath.Empty();
    m_previewTimePosition = 0.0;
    InvalidateRect(&m_rcPreview, FALSE);
}

#ifdef _DEBUG
void NWPVideoEditorView::AssertValid() const {
    CView::AssertValid();
}

void NWPVideoEditorView::Dump(CDumpContext& dc) const {
    CView::Dump(dc);
}

CNWPVideoEditorDoc* NWPVideoEditorView::GetDocument() const {
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNWPVideoEditorDoc)));
    return (CNWPVideoEditorDoc*)m_pDocument;
}
#endif
