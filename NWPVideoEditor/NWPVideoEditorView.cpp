#include "pch.h"
#include "framework.h"
#include "NWPVideoEditor.h"
#include "NWPVideoEditorDoc.h"
#include "NWPVideoEditorView.h"
#include "Resource.h"
#include <shlobj.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(NWPVideoEditorView, CView)

BEGIN_MESSAGE_MAP(NWPVideoEditorView, CView)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_COMMAND(ID_FILE_IMPORT, OnFileImport)
    ON_COMMAND(ID_FILE_OPEN, OnFileImport)
    ON_COMMAND(ID_FILE_EXPORT, OnFileExport)
    ON_COMMAND(ID_EDIT_ADDTEXT, OnEditAddText)
    ON_NOTIFY(NM_DBLCLK, 1001, OnListDblClk)
    ON_NOTIFY(LVN_ITEMACTIVATE, 1001, OnListItemActivate)
    ON_NOTIFY(LVN_BEGINDRAG, 1001, OnListBeginDrag)
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
    ON_WM_RBUTTONDOWN()
    ON_COMMAND(ID_TIMELINE_REMOVE_CLIP, OnTimelineRemoveClip)
    ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
END_MESSAGE_MAP()


NWPVideoEditorView::NWPVideoEditorView() {}

NWPVideoEditorView::~NWPVideoEditorView()
{
    if (m_pDragImage)
    {
        delete m_pDragImage;
        m_pDragImage = nullptr;
    }
}

BOOL NWPVideoEditorView::PreCreateWindow(CREATESTRUCT& cs) {
    return CView::PreCreateWindow(cs);
}

int NWPVideoEditorView::OnCreate(LPCREATESTRUCT cs)
{
    if (CView::OnCreate(cs) == -1) return -1;

    CRect rc(0, 0, 100, 100);
    DWORD style = WS_CHILD | WS_VISIBLE | LVS_ICON | LVS_AUTOARRANGE | LVS_SINGLESEL;
    if (!m_list.Create(style, rc, this, 1001)) return -1;

    m_list.SetExtendedStyle(m_list.GetExtendedStyle()
        | LVS_EX_ONECLICKACTIVATE | LVS_EX_TWOCLICKACTIVATE);

    if (!m_imgLarge.Create(96, 96, ILC_COLOR32 | ILC_MASK, 1, 32)) return -1;
    m_list.SetImageList(&m_imgLarge, LVSIL_NORMAL);

    return 0;
}

void NWPVideoEditorView::OnSize(UINT nType, int cx, int cy)
{
    CView::OnSize(nType, cx, cy);
    Layout(cx, cy);
}

void NWPVideoEditorView::Layout(int cx, int cy)
{
    const int timelineH = 160;
    if (m_list.GetSafeHwnd()) m_list.MoveWindow(0, 0, cx, cy - timelineH);
    m_rcTimeline = CRect(0, cy - timelineH, cx, cy);
    Invalidate(FALSE);
}

int NWPVideoEditorView::AddShellIconForFile(const CString& path)
{
    SHFILEINFO sfi{};
    if (SHGetFileInfo(path, FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi),
        SHGFI_ICON | SHGFI_LARGEICON) == 0)
        return -1;

    int idx = m_imgLarge.Add(sfi.hIcon);
    DestroyIcon(sfi.hIcon);
    return idx;
}

void NWPVideoEditorView::OnFileImport()
{
    CFileDialog dlg(TRUE, NULL, NULL, OFN_FILEMUSTEXIST,
        _T("Media Files|*.mp4;*.mov;*.mkv;*.avi;*.mp3;*.wav|All Files|*.*||"));
    if (dlg.DoModal() != IDOK) return;

    ClipItem ci;
    ci.path = dlg.GetPathName();
    ci.iImage = AddShellIconForFile(ci.path);

    // Get actual video duration instead of hardcoding
    ci.durationSec = GetVideoDuration(ci.path);

    m_importedClips.push_back(ci);

    CString name = dlg.GetFileName();
    int img = (ci.iImage >= 0) ? ci.iImage : 0;
    int row = m_list.InsertItem(m_list.GetItemCount(), name, img);
    m_list.SetItemState(row, LVIS_SELECTED, LVIS_SELECTED);
}

void NWPVideoEditorView::OnRButtonDown(UINT nFlags, CPoint point)
{
    if (!m_rcTimeline.PtInRect(point))
    {
        CView::OnRButtonDown(nFlags, point);
        return;
    }

    // Check if we right-clicked on a timeline clip
    int hitClipIndex = HitTestTimelineClip(point);
    if (hitClipIndex >= 0)
    {
        m_contextMenuClipIndex = hitClipIndex;
        m_activeTimelineClipIndex = hitClipIndex;  // Select the clip
        InvalidateRect(m_rcTimeline, FALSE);

        // Create context menu
        CMenu contextMenu;
        contextMenu.CreatePopupMenu();

        CString clipName = m_timelineClips[hitClipIndex].path;
        int lastSlash = clipName.ReverseFind('\\');
        if (lastSlash != -1) clipName = clipName.Mid(lastSlash + 1);

        CString menuText;
        menuText.Format(_T("Remove \"%s\" from Timeline"), clipName);
        contextMenu.AppendMenu(MF_STRING, ID_TIMELINE_REMOVE_CLIP, menuText);

        // Show context menu
        CPoint screenPoint = point;
        ClientToScreen(&screenPoint);
        contextMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
            screenPoint.x, screenPoint.y, this);
    }
    else
    {
        CView::OnRButtonDown(nFlags, point);
    }
}

void NWPVideoEditorView::OnTimelineRemoveClip()
{
    if (m_contextMenuClipIndex >= 0 && m_contextMenuClipIndex < (int)m_timelineClips.size())
    {
        // Store the clip being removed for confirmation
        CString clipName = m_timelineClips[m_contextMenuClipIndex].path;
        int lastSlash = clipName.ReverseFind('\\');
        if (lastSlash != -1) clipName = clipName.Mid(lastSlash + 1);

        CString message;
        message.Format(_T("Remove \"%s\" from the timeline?"), clipName);

        if (AfxMessageBox(message, MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
            // Remove the clip from timeline
            m_timelineClips.erase(m_timelineClips.begin() + m_contextMenuClipIndex);

            // Adjust active clip index if necessary
            if (m_activeTimelineClipIndex == m_contextMenuClipIndex)
            {
                m_activeTimelineClipIndex = -1;  // Deselect if we removed the active clip
            }
            else if (m_activeTimelineClipIndex > m_contextMenuClipIndex)
            {
                m_activeTimelineClipIndex--;  // Adjust index if active clip was after removed clip
            }

            // Reposition remaining clips to fill the gap (optional)
            RepositionClipsAfterRemoval();

            InvalidateRect(m_rcTimeline, FALSE);
        }
    }

    m_contextMenuClipIndex = -1;  // Reset context menu clip index
}

void NWPVideoEditorView::RepositionClipsAfterRemoval()
{
    // Sort clips by timeline position
    std::sort(m_timelineClips.begin(), m_timelineClips.end(),
        [](const TimelineClip& a, const TimelineClip& b) {
            return a.startTimeOnTimeline < b.startTimeOnTimeline;
        });

    // Reposition clips to remove gaps
    double currentPos = 0.0;
    for (auto& clip : m_timelineClips)
    {
        clip.startTimeOnTimeline = currentPos;
        currentPos += clip.clipLengthSec;
    }
}



void NWPVideoEditorView::SetActiveClipFromSelection()
{
    int sel = m_list.GetNextItem(-1, LVNI_SELECTED);
    if (sel < 0 || sel >= (int)m_importedClips.size()) return;

    // Don't change active timeline clip when selecting from imported list
    InvalidateRect(m_rcTimeline, FALSE);
}

double NWPVideoEditorView::GetVideoDuration(const CString& filePath)
{
    CStringA pathA(filePath);

    // Use ffprobe to get video duration
    CStringA cmd;
    cmd.Format("ffprobe -v error -select_streams v:0 -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 \"%s\"",
        pathA.GetString());

    std::string wrapper = std::string("cmd /C ") + cmd.GetString();
    std::vector<char> mutableCmd(wrapper.begin(), wrapper.end());
    mutableCmd.push_back('\0');

    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
        return 30.0; // Default fallback

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;

    PROCESS_INFORMATION pi{};

    BOOL success = CreateProcessA(nullptr, mutableCmd.data(), nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

    if (!success)
    {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return 30.0; // Default fallback
    }

    CloseHandle(hWritePipe);

    // Read the output
    char buffer[1024];
    DWORD bytesRead;
    std::string output;

    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        output += buffer;
    }

    WaitForSingleObject(pi.hProcess, 5000); // 5 second timeout
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadPipe);

    // Parse the duration
    double duration = 30.0; // Default
    try
    {
        if (!output.empty())
        {
            duration = std::stod(output);
            if (duration <= 0) duration = 30.0;
        }
    }
    catch (...)
    {
        duration = 30.0;
    }

    return duration;
}


void NWPVideoEditorView::AddClipToTimeline(const CString& clipPath)
{
    // Find the clip's original duration
    double originalDuration = 30.0; // default
    int imageIndex = -1;
    for (const auto& clip : m_importedClips) {
        if (clip.path == clipPath) {
            originalDuration = clip.durationSec > 0 ? clip.durationSec : 30.0;
            imageIndex = clip.iImage;
            break;
        }
    }

    // Calculate position for new clip (at the end of existing clips)
    double newStartTime = 0.0;
    for (const auto& timelineClip : m_timelineClips) {
        double clipEnd = timelineClip.startTimeOnTimeline + timelineClip.clipLengthSec;
        if (clipEnd > newStartTime) {
            newStartTime = clipEnd;
        }
    }

    // Create new timeline clip
    TimelineClip newClip;
    newClip.path = clipPath;
    newClip.startTimeOnTimeline = newStartTime;
    newClip.clipStartSec = 0.0;
    newClip.clipLengthSec = min(10.0, originalDuration);
    newClip.originalDuration = originalDuration;
    newClip.iImage = imageIndex;

    m_timelineClips.push_back(newClip);
    m_activeTimelineClipIndex = (int)m_timelineClips.size() - 1;

    // Reset dragging states - REMOVED the ReleaseCapture() call that was preventing multiple clips
    m_dragState = DRAG_NONE;
    m_bDragging = FALSE;

    InvalidateRect(m_rcTimeline, FALSE);
}


void NWPVideoEditorView::OnListDblClk(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMITEMACTIVATE* pNMItemActivate = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    int i = pNMItemActivate->iItem;
    if (i >= 0 && i < (int)m_importedClips.size())
    {
        AddClipToTimeline(m_importedClips[i].path);
    }
    if (pResult) *pResult = 0;
}

void NWPVideoEditorView::OnListItemActivate(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    SetActiveClipFromSelection();
    if (pResult) *pResult = 0;
}

void NWPVideoEditorView::OnListBeginDrag(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMLISTVIEW* pNMListView = (NMLISTVIEW*)pNMHDR;
    m_nDragIndex = pNMListView->iItem;
    if (m_nDragIndex >= 0 && m_nDragIndex < (int)m_importedClips.size())
    {
        POINT pt = { 10, 10 };
        m_pDragImage = m_list.CreateDragImage(m_nDragIndex, &pt);
        if (m_pDragImage)
        {
            m_pDragImage->BeginDrag(0, CPoint(10, 10));
            m_pDragImage->DragEnter(GetDesktopWindow(), pNMListView->ptAction);
            m_bDragging = TRUE;
            SetCapture();
        }
    }
    *pResult = 0;
}

BOOL NWPVideoEditorView::IsOverTimeline(CPoint screenPt)
{
    CPoint clientPt = screenPt;
    ScreenToClient(&clientPt);
    return m_rcTimeline.PtInRect(clientPt);
}

int NWPVideoEditorView::HitTestTimelineClip(CPoint pt) const
{
    if (m_timelineClips.empty()) return -1;

    const CRect& r = m_rcTimeline;
    int left = r.left + 12;
    int right = r.right - 12;
    int top = r.top + 48;
    int bottom = r.bottom - 36;

    for (int i = 0; i < (int)m_timelineClips.size(); i++)
    {
        const TimelineClip& clip = m_timelineClips[i];
        int clipStartX = SecondsToTimelineX(clip.startTimeOnTimeline);
        int clipEndX = SecondsToTimelineX(clip.startTimeOnTimeline + clip.clipLengthSec);

        CRect clipRect(clipStartX, top, clipEndX, bottom);
        if (clipRect.PtInRect(pt))
        {
            return i;
        }
    }
    return -1;
}

int NWPVideoEditorView::HitTestTimelineHandle(CPoint pt) const
{
    if (m_activeTimelineClipIndex < 0 || m_activeTimelineClipIndex >= (int)m_timelineClips.size())
        return 0;

    const TimelineClip& activeClip = m_timelineClips[m_activeTimelineClipIndex];
    const CRect& r = m_rcTimeline;
    int top = r.top + 48;
    int bottom = r.bottom - 36;

    int clipStartX = SecondsToTimelineX(activeClip.startTimeOnTimeline);
    int clipEndX = SecondsToTimelineX(activeClip.startTimeOnTimeline + activeClip.clipLengthSec);

    CRect clipR(clipStartX, top, clipEndX, bottom);

    // Left resize handle
    CRect leftHandleR(clipR.left, clipR.top, clipR.left + 8, clipR.bottom);
    if (leftHandleR.PtInRect(pt)) return 3;    // Left handle

    // Right resize handle  
    CRect rightHandleR(clipR.right - 8, clipR.top, clipR.right, clipR.bottom);
    if (rightHandleR.PtInRect(pt)) return 2;   // Right handle

    if (clipR.PtInRect(pt)) return 1;          // Inside clip
    return 0;
}

double NWPVideoEditorView::TimelineXToSeconds(int x) const
{
    const CRect& r = m_rcTimeline;
    int left = r.left + 12;
    int right = r.right - 12;
    int timelineWidth = right - left;

    if (timelineWidth <= 0) return 0.0;

    double ratio = (double)(x - left) / timelineWidth;
    return ratio * m_timelineDurationSec + m_timelineScrollOffset;
}

int NWPVideoEditorView::SecondsToTimelineX(double seconds) const
{
    const CRect& r = m_rcTimeline;
    int left = r.left + 12;
    int right = r.right - 12;
    int timelineWidth = right - left;

    if (m_timelineDurationSec <= 0) return left;

    double ratio = (seconds - m_timelineScrollOffset) / m_timelineDurationSec;
    return left + (int)(ratio * timelineWidth);
}

void NWPVideoEditorView::OnLButtonDown(UINT nFlags, CPoint pt)
{
    if (!m_rcTimeline.PtInRect(pt))
        return;

    m_dragState = DRAG_NONE;

    // First check if we clicked on a clip
    int hitClipIndex = HitTestTimelineClip(pt);
    if (hitClipIndex >= 0)
    {
        m_activeTimelineClipIndex = hitClipIndex;

        // Now check for handle hits on the active clip
        int hit = HitTestTimelineHandle(pt);
        if (hit == 3) // Left handle
        {
            m_dragState = DRAG_LEFT_HANDLE;
            m_dragStart = pt;
            const TimelineClip& clip = m_timelineClips[m_activeTimelineClipIndex];
            m_dragStartClipStart = clip.clipStartSec;
            m_dragStartClipLength = clip.clipLengthSec;
            m_dragStartTimelinePos = clip.startTimeOnTimeline; // ADDED THIS
            SetCapture();
        }
        else if (hit == 2) // Right handle
        {
            m_dragState = DRAG_RIGHT_HANDLE;
            m_dragStart = pt;
            const TimelineClip& clip = m_timelineClips[m_activeTimelineClipIndex];
            m_dragStartClipStart = clip.clipStartSec;
            m_dragStartClipLength = clip.clipLengthSec;
            SetCapture();
        }
        else if (hit == 1) // Inside clip - prepare for moving
        {
            m_dragState = DRAG_CLIP_MOVE;
            m_dragStart = pt;
            const TimelineClip& clip = m_timelineClips[m_activeTimelineClipIndex];
            m_dragStartTimelinePos = clip.startTimeOnTimeline;
            SetCapture();
        }

        InvalidateRect(m_rcTimeline, FALSE);
    }
    else
    {
        // Clicked on empty timeline area
        m_activeTimelineClipIndex = -1;
        InvalidateRect(m_rcTimeline, FALSE);
    }
}


void NWPVideoEditorView::OnMouseMove(UINT nFlags, CPoint point)
{
    // Handle clip drag-and-drop from list
    if (m_bDragging && m_pDragImage)
    {
        CPoint ptScreen(point);
        ClientToScreen(&ptScreen);
        m_pDragImage->DragMove(ptScreen);
        if (IsOverTimeline(ptScreen))
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        else
            SetCursor(LoadCursor(NULL, IDC_NO));
        return;
    }

    // Handle timeline handle/clip dragging
    if (m_dragState != DRAG_NONE && GetCapture() == this &&
        m_activeTimelineClipIndex >= 0 && m_activeTimelineClipIndex < (int)m_timelineClips.size())
    {
        TimelineClip& activeClip = m_timelineClips[m_activeTimelineClipIndex];

        if (m_dragState == DRAG_LEFT_HANDLE)
        {
            int dx = point.x - m_dragStart.x;
            const CRect& r = m_rcTimeline;
            int timelineWidth = (r.Width() - 24);
            if (timelineWidth <= 0) return;

            double pixelsPerSecond = timelineWidth / m_timelineDurationSec;
            double deltaSeconds = dx / pixelsPerSecond;

            // Move the timeline position and adjust clip start
            double newTimelinePos = max(0.0, m_dragStartTimelinePos + deltaSeconds);
            double newClipStart = max(0.0, min(activeClip.originalDuration - 1.0,
                m_dragStartClipStart - deltaSeconds));

            // Calculate new length to maintain the right edge position
            double rightEdgeTime = m_dragStartTimelinePos + m_dragStartClipLength;
            double newLength = max(1.0, rightEdgeTime - newTimelinePos);

            // PREVENT STRETCHING BEYOND ORIGINAL CLIP LENGTH
            if (newClipStart + newLength > activeClip.originalDuration) {
                newLength = activeClip.originalDuration - newClipStart;
            }

            // Ensure minimum length
            if (newLength < 1.0) {
                newLength = 1.0;
                newClipStart = max(0.0, activeClip.originalDuration - newLength);
            }

            activeClip.startTimeOnTimeline = newTimelinePos;
            activeClip.clipStartSec = newClipStart;
            activeClip.clipLengthSec = newLength;
        }
        else if (m_dragState == DRAG_RIGHT_HANDLE)
        {
            double currentTime = TimelineXToSeconds(point.x);
            double deltaTime = currentTime - TimelineXToSeconds(m_dragStart.x);
            double newLength = m_dragStartClipLength + deltaTime;

            // PREVENT STRETCHING BEYOND ORIGINAL CLIP LENGTH
            double maxLength = activeClip.originalDuration - activeClip.clipStartSec;
            activeClip.clipLengthSec = max(1.0, min(maxLength, newLength));
        }
        else if (m_dragState == DRAG_CLIP_MOVE)
        {
            double currentTime = TimelineXToSeconds(point.x);
            double deltaTime = currentTime - TimelineXToSeconds(m_dragStart.x);
            double newPos = max(0.0, m_dragStartTimelinePos + deltaTime);

            // Simple collision detection - prevent overlap
            bool canMove = true;
            for (int i = 0; i < (int)m_timelineClips.size(); i++)
            {
                if (i == m_activeTimelineClipIndex) continue;
                const TimelineClip& otherClip = m_timelineClips[i];
                double otherStart = otherClip.startTimeOnTimeline;
                double otherEnd = otherClip.startTimeOnTimeline + otherClip.clipLengthSec;
                double thisEnd = newPos + activeClip.clipLengthSec;

                if (!(thisEnd <= otherStart || newPos >= otherEnd))
                {
                    canMove = false;
                    break;
                }
            }

            if (canMove)
            {
                activeClip.startTimeOnTimeline = newPos;
            }
        }

        InvalidateRect(m_rcTimeline, FALSE);
        return;
    }

    // Cursor feedback when not dragging
    if (m_rcTimeline.PtInRect(point) && m_dragState == DRAG_NONE)
    {
        int hitClipIndex = HitTestTimelineClip(point);
        if (hitClipIndex >= 0)
        {
            int oldActive = m_activeTimelineClipIndex;
            m_activeTimelineClipIndex = hitClipIndex;
            int hit = HitTestTimelineHandle(point);
            m_activeTimelineClipIndex = oldActive; // Don't change selection just for cursor

            if (hit == 2 || hit == 3)
                SetCursor(LoadCursor(NULL, IDC_SIZEWE));
            else if (hit == 1)
                SetCursor(LoadCursor(NULL, IDC_SIZEALL));
            else
                SetCursor(LoadCursor(NULL, IDC_ARROW));
        }
        else
        {
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        }
    }
}


void NWPVideoEditorView::OnLButtonUp(UINT nFlags, CPoint point)
{
    // Handle drag-drop completion from list
    if (m_bDragging)
    {
        if (GetCapture() == this) ReleaseCapture();
        m_bDragging = FALSE;
        if (m_pDragImage)
        {
            m_pDragImage->DragLeave(GetDesktopWindow());
            m_pDragImage->EndDrag();
            delete m_pDragImage;
            m_pDragImage = nullptr;
        }

        CPoint ptScreen(point);
        ClientToScreen(&ptScreen);
        if (IsOverTimeline(ptScreen) && m_nDragIndex >= 0 && m_nDragIndex < (int)m_importedClips.size())
        {
            AddClipToTimeline(m_importedClips[m_nDragIndex].path);
        }
        m_nDragIndex = -1;
        return;
    }

    // Handle timeline dragging completion
    if (m_dragState != DRAG_NONE)
    {
        if (GetCapture() == this) ReleaseCapture();
        m_dragState = DRAG_NONE;
        InvalidateRect(m_rcTimeline, FALSE);
    }
}

void NWPVideoEditorView::OnDraw(CDC* pDC)
{
    DrawTimeline(pDC);
}

void NWPVideoEditorView::DrawTimeline(CDC* pDC)
{
    CRect r = m_rcTimeline;
    pDC->FillSolidRect(r, RGB(28, 28, 28));
    pDC->DrawEdge(r, EDGE_SUNKEN, BF_RECT);

    pDC->SetTextColor(RGB(230, 230, 230));
    pDC->TextOut(r.left + 10, r.top + 10, _T("Timeline"));

    if (m_timelineClips.empty()) {
        pDC->TextOut(r.left + 10, r.top + 30, _T("Double-click or drag clips above to place them on the timeline."));
        return;
    }

    int left = r.left + 12;
    int right = r.right - 12;
    int top = r.top + 48;
    int bottom = r.bottom - 36;

    // Draw all timeline clips
    for (int i = 0; i < (int)m_timelineClips.size(); i++)
    {
        const TimelineClip& clip = m_timelineClips[i];

        int clipStartX = SecondsToTimelineX(clip.startTimeOnTimeline);
        int clipEndX = SecondsToTimelineX(clip.startTimeOnTimeline + clip.clipLengthSec);

        CRect clipR(clipStartX, top, clipEndX, bottom);

        // Different colors for active vs inactive clips
        COLORREF clipColor = (i == m_activeTimelineClipIndex) ? RGB(70, 120, 220) : RGB(90, 90, 120);
        COLORREF borderColor = (i == m_activeTimelineClipIndex) ? RGB(30, 60, 140) : RGB(60, 60, 80);

        pDC->FillSolidRect(clipR, clipColor);
        pDC->FrameRect(clipR, &CBrush(borderColor));

        // Draw handles only for active clip
        if (i == m_activeTimelineClipIndex)
        {
            // Left handle
            CRect leftHandleR(clipR.left, clipR.top, clipR.left + 8, clipR.bottom);
            pDC->FillSolidRect(leftHandleR, RGB(255, 160, 160));

            // Right handle
            CRect rightHandleR(clipR.right - 8, clipR.top, clipR.right, clipR.bottom);
            pDC->FillSolidRect(rightHandleR, RGB(160, 200, 255));
        }

        // Clip text
        CString clipText;
        CString filename = clip.path;
        int lastSlash = filename.ReverseFind('\\');
        if (lastSlash != -1) filename = filename.Mid(lastSlash + 1);

        clipText.Format(_T("%.1fs"), clip.clipLengthSec);

        CRect textR = clipR;
        textR.DeflateRect(12, 2);
        pDC->SetBkMode(TRANSPARENT);
        pDC->SetTextColor(RGB(255, 255, 255));
        pDC->DrawText(clipText, textR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // Draw overlays
    for (const auto& ov : m_overlays) {
        int x1 = SecondsToTimelineX(ov.startSec);
        int x2 = SecondsToTimelineX(ov.startSec + ov.durSec);
        if (x2 > x1) {
            CRect ovR(x1, r.bottom - 24, x2, r.bottom - 4);
            pDC->FillSolidRect(ovR, RGB(255, 200, 100));
            pDC->FrameRect(ovR, &CBrush(RGB(200, 160, 80)));
            pDC->SetTextColor(RGB(0, 0, 0));
            pDC->DrawText(ov.text, ovR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    }

    // Draw timeline info for active clip
    if (m_activeTimelineClipIndex >= 0 && m_activeTimelineClipIndex < (int)m_timelineClips.size())
    {
        const TimelineClip& activeClip = m_timelineClips[m_activeTimelineClipIndex];
        CString infoText;
        infoText.Format(_T("Active Clip - Timeline Pos: %.1fs | Clip Start: %.1fs | Length: %.1fs"),
            activeClip.startTimeOnTimeline, activeClip.clipStartSec, activeClip.clipLengthSec);
        pDC->SetTextColor(RGB(200, 255, 200));
        pDC->TextOut(r.left + 10, r.top + 30, infoText);
    }
}

void NWPVideoEditorView::OnFileExport()
{
    if (m_timelineClips.empty()) {
        AfxMessageBox(L"Add clips to the timeline first.");
        return;
    }

    CFileDialog sfd(FALSE, L"mp4", L"output.mp4", OFN_OVERWRITEPROMPT,
        L"MP4 video|*.mp4||");
    if (sfd.DoModal() != IDOK) return;

    CString out = sfd.GetPathName();

    // For now, export the first clip (you can extend this to handle multiple clips)
    const TimelineClip& firstClip = m_timelineClips[0];

    CStringA inA(firstClip.path);
    CStringA outA(out);
    CStringA ff;
    ff.Format("ffmpeg -y -ss %.2f -i \"%s\" -t %.2f -c:v libx264 -preset veryfast -crf 20 -c:a aac \"%s\"",
        firstClip.clipStartSec, inA.GetString(), firstClip.clipLengthSec, outA.GetString());

    std::string wrapper = std::string("cmd /C ") + ff.GetString();
    std::vector<char> mutableCmd(wrapper.begin(), wrapper.end());
    mutableCmd.push_back('\0');

    STARTUPINFOA si{}; si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    BOOL ok = CreateProcessA(nullptr, mutableCmd.data(), nullptr, nullptr, FALSE,
        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

    if (!ok) { AfxMessageBox(L"Failed to start ffmpeg. Ensure it's in PATH."); return; }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0; GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess);

    if (exitCode == 0) AfxMessageBox(L"Export finished.");
    else AfxMessageBox(L"Export failed.");
}

void NWPVideoEditorView::OnEditAddText()
{
    CString text = L"Sample Text";
    if (AfxMessageBox(L"Add text overlay?", MB_YESNO) == IDYES) {
        OverlayItem ov;
        ov.text = text;
        ov.startSec = 2.0;
        ov.durSec = 3.0;
        m_overlays.push_back(ov);
        InvalidateRect(m_rcTimeline, FALSE);
    }
}

BOOL NWPVideoEditorView::OnPreparePrinting(CPrintInfo* pInfo)
{
    return DoPreparePrinting(pInfo);
}

void NWPVideoEditorView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

void NWPVideoEditorView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

#ifdef _DEBUG
void NWPVideoEditorView::AssertValid() const
{
    CView::AssertValid();
}

void NWPVideoEditorView::Dump(CDumpContext& dc) const
{
    CView::Dump(dc);
}

CNWPVideoEditorDoc* NWPVideoEditorView::GetDocument() const
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNWPVideoEditorDoc)));
    return (CNWPVideoEditorDoc*)m_pDocument;
}
#endif
