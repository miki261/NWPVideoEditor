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
    ON_COMMAND(ID_FILE_SAVE, OnFileSave)
    ON_COMMAND(ID_EDIT_ADDTEXT, OnEditAddText)
    ON_BN_CLICKED(ID_ADD_TEXT_BUTTON, OnAddText)
    ON_NOTIFY(NM_DBLCLK, 1001, OnListDblClk)
    ON_NOTIFY(LVN_ITEMACTIVATE, 1001, OnListItemActivate)
    ON_NOTIFY(LVN_BEGINDRAG, 1001, OnListBeginDrag)
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
    ON_WM_RBUTTONDOWN()
    ON_WM_CONTEXTMENU()
    ON_COMMAND(ID_TIMELINE_REMOVE_CLIP, OnTimelineRemoveClip)
    ON_COMMAND(ID_TIMELINE_SPLIT_CLIP, OnTimelineSplitClip)
    ON_COMMAND(32775, OnDeleteTextOverlay)
    ON_BN_CLICKED(ID_PLAY_PAUSE_BUTTON, OnPlayPause)
    ON_BN_CLICKED(ID_STOP_BUTTON, OnStop)
    ON_WM_TIMER()
    ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
END_MESSAGE_MAP()

NWPVideoEditorView::NWPVideoEditorView()
{
    m_hPreviewBitmap = nullptr;
    m_previewTimePosition = 0.0;
}

NWPVideoEditorView::~NWPVideoEditorView()
{
    if (m_playbackTimer)
    {
        KillTimer(m_playbackTimer);
        m_playbackTimer = 0;
    }
    if (m_pDragImage)
    {
        delete m_pDragImage;
        m_pDragImage = nullptr;
    }
    if (m_hPreviewBitmap)
    {
        DeleteObject(m_hPreviewBitmap);
        m_hPreviewBitmap = nullptr;
    }
}

BOOL NWPVideoEditorView::PreCreateWindow(CREATESTRUCT& cs)
{
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

    m_rcPreview = CRect(0, 0, 480, 360);

    if (!m_playPauseButton.Create(_T("Play"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        CRect(0, 0, 60, 25), this, ID_PLAY_PAUSE_BUTTON))
        return -1;

    if (!m_stopButton.Create(_T("Stop"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        CRect(0, 0, 60, 25), this, ID_STOP_BUTTON))
        return -1;

    if (!m_addTextButton.Create(_T("Add Text"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        CRect(0, 0, 70, 25), this, ID_ADD_TEXT_BUTTON))
        return -1;

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
    const int previewW = 480;
    const int previewH = 360;
    const int margin = 10;
    const int buttonH = 25;
    const int buttonW = 60;
    const int addTextButtonW = 70;
    const int buttonSpacing = 10;

    if (m_list.GetSafeHwnd())
    {
        int listWidth = cx - previewW - margin * 3;
        m_list.MoveWindow(margin, margin, listWidth, cy - timelineH - margin * 2);
    }

    int previewX = cx - previewW - margin;
    int previewY = margin;
    m_rcPreview = CRect(previewX, previewY, previewX + previewW, previewY + previewH);

    if (m_playPauseButton.GetSafeHwnd())
    {
        int buttonY = previewY + previewH + 5;
        m_playPauseButton.MoveWindow(previewX, buttonY, buttonW, buttonH);
        m_stopButton.MoveWindow(previewX + buttonW + buttonSpacing, buttonY, buttonW, buttonH);
        m_addTextButton.MoveWindow(previewX + (buttonW + buttonSpacing) * 2, buttonY, addTextButtonW, buttonH);
    }

    m_rcTimeline = CRect(margin, cy - timelineH, cx - margin, cy - margin);
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
    const int MAX_FILES = 100;
    const int BUFFER_SIZE = MAX_FILES * MAX_PATH + 1;

    CString fileBuffer;
    TCHAR* pBuffer = fileBuffer.GetBuffer(BUFFER_SIZE);
    pBuffer[0] = '\0';

    CFileDialog dlg(TRUE, NULL, NULL,
        OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER,
        _T("Media Files|*.mp4;*.mov;*.mkv;*.avi;*.mp3;*.wav|All Files|*.*||"));

    dlg.m_ofn.lpstrFile = pBuffer;
    dlg.m_ofn.nMaxFile = BUFFER_SIZE;

    if (dlg.DoModal() != IDOK)
    {
        fileBuffer.ReleaseBuffer();
        return;
    }

    fileBuffer.ReleaseBuffer();

    POSITION pos = dlg.GetStartPosition();
    std::vector<CString> selectedFiles;

    while (pos != NULL)
    {
        CString filePath = dlg.GetNextPathName(pos);
        selectedFiles.push_back(filePath);
    }

    for (const CString& filePath : selectedFiles)
    {
        ClipItem ci;
        ci.path = filePath;
        ci.iImage = AddShellIconForFile(ci.path);
        ci.durationSec = GetVideoDuration(ci.path);
        m_importedClips.push_back(ci);

        CString name = filePath;
        int lastSlash = name.ReverseFind('\\');
        if (lastSlash != -1) name = name.Mid(lastSlash + 1);

        int img = (ci.iImage >= 0) ? ci.iImage : 0;
        int row = m_list.InsertItem(m_list.GetItemCount(), name, img);

        if (filePath == selectedFiles.back())
        {
            m_list.SetItemState(row, LVIS_SELECTED, LVIS_SELECTED);
        }
    }

    UpdatePreview();

    if (selectedFiles.size() > 1)
    {
        CString message;
        message.Format(_T("Successfully imported %d files."), (int)selectedFiles.size());
        AfxMessageBox(message);
    }
}

void NWPVideoEditorView::OnAddText()
{
    CTextInputDialog dlg;

    if (dlg.DoModal(this) == IDOK)
    {
        TextOverlay overlay;
        overlay.text = dlg.GetText();
        overlay.startSec = 0.0;
        overlay.durSec = 5.0;
        overlay.position = CPoint(50, 50);

        m_textOverlays.push_back(overlay);
        m_activeTextOverlayIndex = (int)m_textOverlays.size() - 1;

        InvalidateRect(m_rcTimeline, FALSE);
        InvalidateRect(m_rcPreview, FALSE);

        CString message;
        message.Format(_T("Text \"%s\" added! Click and drag it in the preview to reposition."), overlay.text);
        AfxMessageBox(message);
    }
}

int NWPVideoEditorView::HitTestTimelineTextOverlay(CPoint pt) const
{
    if (!m_rcTimeline.PtInRect(pt)) return -1;

    const CRect& r = m_rcTimeline;
    int textTrackTop = r.bottom - 50;
    int textTrackBottom = r.bottom - 10;

    if (pt.y < textTrackTop || pt.y > textTrackBottom) return -1;

    for (int i = 0; i < (int)m_textOverlays.size(); i++)
    {
        const TextOverlay& overlay = m_textOverlays[i];
        int startX = SecondsToTimelineX(overlay.startSec);
        int endX = SecondsToTimelineX(overlay.startSec + overlay.durSec);

        if (pt.x >= startX && pt.x <= endX)
            return i;
    }
    return -1;
}

int NWPVideoEditorView::HitTestTextOverlayHandles(CPoint pt) const
{
    if (m_activeTextOverlayIndex < 0 || m_activeTextOverlayIndex >= (int)m_textOverlays.size())
        return 0;

    const TextOverlay& overlay = m_textOverlays[m_activeTextOverlayIndex];
    const CRect& r = m_rcTimeline;
    int textTrackTop = r.bottom - 50;
    int textTrackBottom = r.bottom - 10;

    int startX = SecondsToTimelineX(overlay.startSec);
    int endX = SecondsToTimelineX(overlay.startSec + overlay.durSec);

    if (pt.x >= startX && pt.x <= startX + 8 && pt.y >= textTrackTop && pt.y <= textTrackBottom)
        return 1;

    if (pt.x >= endX - 8 && pt.x <= endX && pt.y >= textTrackTop && pt.y <= textTrackBottom)
        return 2;

    return 0;
}

int NWPVideoEditorView::HitTestTextOverlayInPreview(CPoint pt) const
{
    if (!m_rcPreview.PtInRect(pt)) return -1;

    CRect previewInner = m_rcPreview;
    previewInner.DeflateRect(2, 2);

    CPoint relativePt(pt.x - previewInner.left, pt.y - previewInner.top);

    for (int i = (int)m_textOverlays.size() - 1; i >= 0; i--)
    {
        const TextOverlay& overlay = m_textOverlays[i];

        CRect textBounds(overlay.position.x - 10, overlay.position.y - 10,
            overlay.position.x + 200, overlay.position.y + 40);

        if (textBounds.PtInRect(relativePt))
            return i;
    }

    return -1;
}

void NWPVideoEditorView::DrawTextOverlays(CDC* pDC)
{
    CRect previewInner = m_rcPreview;
    previewInner.DeflateRect(2, 2);

    for (int i = 0; i < (int)m_textOverlays.size(); i++)
    {
        const TextOverlay& overlay = m_textOverlays[i];

        CFont font;
        font.CreateFont(overlay.fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, 0,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS, overlay.fontName);

        CFont* oldFont = pDC->SelectObject(&font);
        pDC->SetBkMode(TRANSPARENT);

        CPoint textPos(previewInner.left + overlay.position.x, previewInner.top + overlay.position.y);

        pDC->SetTextColor(RGB(0, 0, 0));
        pDC->TextOut(textPos.x + 3, textPos.y + 3, overlay.text);

        pDC->SetTextColor(overlay.color);
        pDC->TextOut(textPos.x, textPos.y, overlay.text);

        if (i == m_draggingTextIndex)
        {
            CSize textSize = pDC->GetTextExtent(overlay.text);
            CRect selRect(textPos.x - 5, textPos.y - 5,
                textPos.x + textSize.cx + 5, textPos.y + textSize.cy + 5);
            CPen selPen(PS_DOT, 1, RGB(255, 255, 0));
            CPen* oldPen = pDC->SelectObject(&selPen);
            CBrush* oldBrush = (CBrush*)pDC->SelectStockObject(NULL_BRUSH);
            pDC->Rectangle(selRect);
            pDC->SelectObject(oldPen);
            pDC->SelectObject(oldBrush);
        }

        pDC->SelectObject(oldFont);
    }
}

CStringA NWPVideoEditorView::BuildTextOverlayFilter() const
{
    if (m_textOverlays.empty()) return "";

    CStringA filter;

    for (int i = 0; i < (int)m_textOverlays.size(); i++)
    {
        const TextOverlay& overlay = m_textOverlays[i];

        CStringA text(overlay.text);
        text.Replace(":", "\\:");
        text.Replace("'", "\\'");

        int x = (overlay.position.x * 480) / 476;
        int y = (overlay.position.y * 360) / 356;

        CStringA overlayFilter;
        overlayFilter.Format("drawtext=text='%s':fontsize=%d:fontcolor=white:x=%d:y=%d:enable='between(t,%.2f,%.2f)'",
            text.GetString(),
            overlay.fontSize,
            x, y,
            overlay.startSec,
            overlay.startSec + overlay.durSec);

        if (i > 0) filter += ",";
        filter += overlayFilter;
    }

    return filter;
}

void NWPVideoEditorView::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
    ScreenToClient(&point);

    int textHit = HitTestTimelineTextOverlay(point);
    if (textHit >= 0)
    {
        m_activeTextOverlayIndex = textHit;

        CMenu menu;
        menu.CreatePopupMenu();

        CString menuText;
        menuText.Format(_T("Delete \"%s\""), m_textOverlays[textHit].text);
        menu.AppendMenu(MF_STRING, 32775, menuText);

        CPoint screenPt = point;
        ClientToScreen(&screenPt);
        menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, screenPt.x, screenPt.y, this);
        return;
    }

    if (m_rcTimeline.PtInRect(point))
    {
        OnRButtonDown(0, point);
    }
}

void NWPVideoEditorView::OnDeleteTextOverlay()
{
    if (m_activeTextOverlayIndex >= 0 && m_activeTextOverlayIndex < (int)m_textOverlays.size())
    {
        CString message;
        message.Format(_T("Delete text overlay \"%s\"?"), m_textOverlays[m_activeTextOverlayIndex].text);

        if (AfxMessageBox(message, MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
            m_textOverlays.erase(m_textOverlays.begin() + m_activeTextOverlayIndex);
            m_activeTextOverlayIndex = -1;
            InvalidateRect(m_rcTimeline, FALSE);
            InvalidateRect(m_rcPreview, FALSE);
            AfxMessageBox(_T("Text overlay deleted."));
        }
    }
}

void NWPVideoEditorView::OnLButtonDown(UINT nFlags, CPoint pt)
{
    int previewTextHit = HitTestTextOverlayInPreview(pt);
    if (previewTextHit >= 0)
    {
        m_draggingTextIndex = previewTextHit;
        m_dragState = DRAG_TEXT_PREVIEW;
        m_textDragStart = pt;
        SetCapture();
        InvalidateRect(m_rcPreview, FALSE);
        return;
    }

    int timelineTextHit = HitTestTimelineTextOverlay(pt);
    if (timelineTextHit >= 0)
    {
        m_activeTextOverlayIndex = timelineTextHit;

        int handleHit = HitTestTextOverlayHandles(pt);
        if (handleHit == 1)
        {
            m_dragState = DRAG_TEXT_LEFT_HANDLE;
            m_dragStart = pt;
            m_dragStartTextStart = m_textOverlays[timelineTextHit].startSec;
            m_dragStartTextDur = m_textOverlays[timelineTextHit].durSec;
            SetCapture();
        }
        else if (handleHit == 2)
        {
            m_dragState = DRAG_TEXT_RIGHT_HANDLE;
            m_dragStart = pt;
            m_dragStartTextStart = m_textOverlays[timelineTextHit].startSec;
            m_dragStartTextDur = m_textOverlays[timelineTextHit].durSec;
            SetCapture();
        }
        else
        {
            m_dragState = DRAG_TEXT_TIMELINE;
            m_dragStart = pt;
            m_dragStartTimelinePos = m_textOverlays[timelineTextHit].startSec;
            SetCapture();
        }

        InvalidateRect(m_rcTimeline, FALSE);
        return;
    }

    if (!m_rcTimeline.PtInRect(pt))
        return;

    m_dragState = DRAG_NONE;
    int hitClipIndex = HitTestTimelineClip(pt);
    if (hitClipIndex >= 0)
    {
        m_activeTimelineClipIndex = hitClipIndex;
        UpdatePreview();
        int hit = HitTestTimelineHandle(pt);
        if (hit == 3)
        {
            m_dragState = DRAG_LEFT_HANDLE;
            m_dragStart = pt;
            const TimelineClip& clip = m_timelineClips[m_activeTimelineClipIndex];
            m_dragStartClipStart = clip.clipStartSec;
            m_dragStartClipLength = clip.clipLengthSec;
            m_dragStartTimelinePos = clip.startTimeOnTimeline;
            SetCapture();
        }
        else if (hit == 2)
        {
            m_dragState = DRAG_RIGHT_HANDLE;
            m_dragStart = pt;
            const TimelineClip& clip = m_timelineClips[m_activeTimelineClipIndex];
            m_dragStartClipStart = clip.clipStartSec;
            m_dragStartClipLength = clip.clipLengthSec;
            SetCapture();
        }
        else if (hit == 1)
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
        m_activeTimelineClipIndex = -1;
        UpdatePreview();
        InvalidateRect(m_rcTimeline, FALSE);
    }
}

void NWPVideoEditorView::OnMouseMove(UINT nFlags, CPoint point)
{
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

    if (m_dragState == DRAG_TEXT_PREVIEW && GetCapture() == this && m_draggingTextIndex >= 0)
    {
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

        InvalidateRect(m_rcPreview, FALSE);
        return;
    }

    if (m_dragState == DRAG_TEXT_LEFT_HANDLE || m_dragState == DRAG_TEXT_RIGHT_HANDLE)
    {
        if (m_activeTextOverlayIndex >= 0 && GetCapture() == this)
        {
            TextOverlay& overlay = m_textOverlays[m_activeTextOverlayIndex];
            double deltaTime = TimelineXToSeconds(point.x) - TimelineXToSeconds(m_dragStart.x);

            if (m_dragState == DRAG_TEXT_LEFT_HANDLE)
            {
                double newStart = max(0.0, m_dragStartTextStart + deltaTime);
                double newDur = max(0.5, m_dragStartTextDur - deltaTime);
                overlay.startSec = newStart;
                overlay.durSec = newDur;
            }
            else if (m_dragState == DRAG_TEXT_RIGHT_HANDLE)
            {
                overlay.durSec = max(0.5, m_dragStartTextDur + deltaTime);
            }

            InvalidateRect(m_rcTimeline, FALSE);
            return;
        }
    }

    if (m_dragState == DRAG_TEXT_TIMELINE && GetCapture() == this &&
        m_activeTextOverlayIndex >= 0 && m_activeTextOverlayIndex < (int)m_textOverlays.size())
    {
        TextOverlay& overlay = m_textOverlays[m_activeTextOverlayIndex];
        double deltaTime = TimelineXToSeconds(point.x) - TimelineXToSeconds(m_dragStart.x);
        double newStartTime = max(0.0, m_dragStartTimelinePos + deltaTime);

        overlay.startSec = newStartTime;

        InvalidateRect(m_rcTimeline, FALSE);
        return;
    }

    if (m_dragState != DRAG_NONE && GetCapture() == this &&
        m_activeTimelineClipIndex >= 0 && m_activeTimelineClipIndex < (int)m_timelineClips.size())
    {
        TimelineClip& activeClip = m_timelineClips[m_activeTimelineClipIndex];

        if (m_dragState == DRAG_LEFT_HANDLE)
        {
            double newTimelinePos = m_dragStartTimelinePos + TimelineXToSeconds(point.x) - TimelineXToSeconds(m_dragStart.x);
            if (newTimelinePos < 0.0)
            {
                newTimelinePos = 0.0;
                activeClip.startTimeOnTimeline = newTimelinePos;
            }
            else
            {
                double deltaSeconds = TimelineXToSeconds(point.x) - TimelineXToSeconds(m_dragStart.x);
                double newClipStart = max(0.0, min(activeClip.originalDuration - 1.0,
                    m_dragStartClipStart - deltaSeconds));
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
        }
        else if (m_dragState == DRAG_RIGHT_HANDLE)
        {
            double currentTime = TimelineXToSeconds(point.x);
            double deltaTime = currentTime - TimelineXToSeconds(m_dragStart.x);
            double newLength = m_dragStartClipLength + deltaTime;
            double maxLength = activeClip.originalDuration - activeClip.clipStartSec;
            activeClip.clipLengthSec = max(1.0, min(maxLength, newLength));
        }
        else if (m_dragState == DRAG_CLIP_MOVE)
        {
            double currentTime = TimelineXToSeconds(point.x);
            double deltaTime = currentTime - TimelineXToSeconds(m_dragStart.x);
            double newPos = max(0.0, m_dragStartTimelinePos + deltaTime);
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

    if (m_rcTimeline.PtInRect(point) && m_dragState == DRAG_NONE)
    {
        int hitClipIndex = HitTestTimelineClip(point);
        if (hitClipIndex >= 0)
        {
            m_hoverClipIndex = hitClipIndex;
            const TimelineClip& clip = m_timelineClips[hitClipIndex];

            int clipStartX = SecondsToTimelineX(clip.startTimeOnTimeline);
            int clipEndX = SecondsToTimelineX(clip.startTimeOnTimeline + clip.clipLengthSec);

            double clipRatio = (double)(point.x - clipStartX) / (clipEndX - clipStartX);
            clipRatio = max(0.0, min(1.0, clipRatio));

            m_hoverTimePosition = clip.clipStartSec + (clipRatio * clip.clipLengthSec);
            m_showSelectionBar = true;

            if (hitClipIndex == m_activeTimelineClipIndex)
            {
                int hit = HitTestTimelineHandle(point);
                if (hit == 2 || hit == 3)
                    SetCursor(LoadCursor(NULL, IDC_SIZEWE));
                else if (hit == 1)
                    SetCursor(LoadCursor(NULL, IDC_SIZEALL));
                else
                    SetCursor(LoadCursor(NULL, IDC_IBEAM));
            }
            else
            {
                SetCursor(LoadCursor(NULL, IDC_IBEAM));
            }

            InvalidateRect(m_rcTimeline, FALSE);
        }
        else
        {
            if (m_showSelectionBar)
            {
                m_showSelectionBar = false;
                m_hoverClipIndex = -1;
                m_hoverTimePosition = -1.0;
                InvalidateRect(m_rcTimeline, FALSE);
            }
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        }
    }
    else
    {
        if (m_showSelectionBar)
        {
            m_showSelectionBar = false;
            m_hoverClipIndex = -1;
            m_hoverTimePosition = -1.0;
            InvalidateRect(m_rcTimeline, FALSE);
        }
    }

    // Change cursor when hovering over text in preview
    if (m_rcPreview.PtInRect(point) && HitTestTextOverlayInPreview(point) >= 0)
    {
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));
    }
}

void NWPVideoEditorView::OnLButtonUp(UINT nFlags, CPoint point)
{
    if (m_dragState == DRAG_TEXT_PREVIEW)
    {
        if (GetCapture() == this) ReleaseCapture();
        m_dragState = DRAG_NONE;
        m_draggingTextIndex = -1;
        InvalidateRect(m_rcPreview, FALSE);
        return;
    }

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
    DrawPreviewFrame(pDC);
}

void NWPVideoEditorView::DrawPreviewFrame(CDC* pDC)
{
    if (m_rcPreview.IsRectEmpty()) return;
    pDC->FillSolidRect(m_rcPreview, RGB(20, 20, 20));
    pDC->Draw3dRect(m_rcPreview, RGB(80, 80, 80), RGB(40, 40, 40));
    CRect innerRect = m_rcPreview;
    innerRect.DeflateRect(2, 2);
    if (m_hPreviewBitmap)
    {
        CDC memDC;
        memDC.CreateCompatibleDC(pDC);
        HBITMAP oldBitmap = (HBITMAP)memDC.SelectObject(m_hPreviewBitmap);
        BITMAP bmp;
        GetObject(m_hPreviewBitmap, sizeof(BITMAP), &bmp);
        pDC->StretchBlt(innerRect.left, innerRect.top, innerRect.Width(), innerRect.Height(),
            &memDC, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
        memDC.SelectObject(oldBitmap);
    }
    else
    {
        pDC->FillSolidRect(innerRect, RGB(40, 40, 40));
        pDC->SetTextColor(RGB(150, 150, 150));
        pDC->SetBkMode(TRANSPARENT);
        pDC->DrawText(_T("Loading frame..."), innerRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    DrawTextOverlays(pDC);

    CRect infoRect = m_rcPreview;
    infoRect.top = infoRect.bottom + 35;
    infoRect.bottom = infoRect.top + 20;
    if (!m_currentPreviewPath.IsEmpty())
    {
        CString filename = m_currentPreviewPath;
        int lastSlash = filename.ReverseFind('\\');
        if (lastSlash != -1) filename = filename.Mid(lastSlash + 1);
        CString info;
        info.Format(_T("%s @ %.2fs %s"), filename, m_previewTimePosition,
            m_isPlaying ? _T("(Playing)") : _T(""));
        pDC->SetTextColor(RGB(200, 200, 200));
        pDC->SetBkMode(TRANSPARENT);
        pDC->DrawText(info, infoRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
}

void NWPVideoEditorView::DrawTimeline(CDC* pDC)
{
    CRect r = m_rcTimeline;
    pDC->FillSolidRect(r, RGB(28, 28, 28));
    pDC->DrawEdge(r, EDGE_SUNKEN, BF_RECT);
    pDC->SetTextColor(RGB(230, 230, 230));
    pDC->TextOut(r.left + 10, r.top + 10, _T("Timeline"));

    if (m_timelineClips.empty() && m_textOverlays.empty()) {
        pDC->TextOut(r.left + 10, r.top + 30, _T("Double-click or drag clips above to place them on the timeline."));
        return;
    }

    int top = r.top + 48;
    int bottom = r.bottom - 56;

    for (int i = 0; i < (int)m_timelineClips.size(); i++)
    {
        const TimelineClip& clip = m_timelineClips[i];
        int clipStartX = SecondsToTimelineX(clip.startTimeOnTimeline);
        int clipEndX = SecondsToTimelineX(clip.startTimeOnTimeline + clip.clipLengthSec);
        CRect clipR(clipStartX, top, clipEndX, bottom);

        COLORREF clipColor = (i == m_activeTimelineClipIndex) ? RGB(70, 120, 220) : RGB(90, 90, 120);
        COLORREF borderColor = (i == m_activeTimelineClipIndex) ? RGB(30, 60, 140) : RGB(60, 60, 80);

        pDC->FillSolidRect(clipR, clipColor);
        pDC->FrameRect(clipR, &CBrush(borderColor));

        if (m_showSelectionBar && m_hoverClipIndex == i)
        {
            double clipRatio = (m_hoverTimePosition - clip.clipStartSec) / clip.clipLengthSec;
            clipRatio = max(0.0, min(1.0, clipRatio));
            int selectionX = clipStartX + (int)(clipRatio * (clipEndX - clipStartX));

            CPen selectionPen(PS_SOLID, 2, RGB(255, 255, 0));
            CPen* oldPen = pDC->SelectObject(&selectionPen);
            pDC->MoveTo(selectionX, clipR.top);
            pDC->LineTo(selectionX, clipR.bottom);
            pDC->SelectObject(oldPen);

            CString timeText;
            timeText.Format(_T("%.2fs"), m_hoverTimePosition);
            CRect timeRect(selectionX - 30, clipR.top - 20, selectionX + 30, clipR.top - 2);
            pDC->SetBkColor(RGB(255, 255, 0));
            pDC->SetTextColor(RGB(0, 0, 0));
            pDC->DrawText(timeText, timeRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        if (i == m_activeTimelineClipIndex)
        {
            CRect leftHandleR(clipR.left, clipR.top, clipR.left + 8, clipR.bottom);
            pDC->FillSolidRect(leftHandleR, RGB(255, 160, 160));
            CRect rightHandleR(clipR.right - 8, clipR.top, clipR.right, clipR.bottom);
            pDC->FillSolidRect(rightHandleR, RGB(160, 200, 255));
        }

        CString clipText;
        clipText.Format(_T("%.1fs"), clip.clipLengthSec);
        CRect textR = clipR;
        textR.DeflateRect(12, 2);
        pDC->SetBkMode(TRANSPARENT);
        pDC->SetTextColor(RGB(255, 255, 255));
        pDC->DrawText(clipText, textR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    int textTrackTop = r.bottom - 50;
    int textTrackBottom = r.bottom - 10;

    pDC->SetTextColor(RGB(200, 200, 200));
    pDC->TextOut(r.left + 10, textTrackTop - 15, _T("Text Overlays:"));

    for (int i = 0; i < (int)m_textOverlays.size(); i++)
    {
        const TextOverlay& overlay = m_textOverlays[i];
        int startX = SecondsToTimelineX(overlay.startSec);
        int endX = SecondsToTimelineX(overlay.startSec + overlay.durSec);

        CRect textRect(startX, textTrackTop, endX, textTrackBottom);

        COLORREF textColor = (i == m_activeTextOverlayIndex) ? RGB(255, 200, 100) : RGB(200, 150, 100);
        COLORREF borderColor = (i == m_activeTextOverlayIndex) ? RGB(200, 160, 80) : RGB(150, 120, 80);

        pDC->FillSolidRect(textRect, textColor);
        pDC->FrameRect(textRect, &CBrush(borderColor));

        if (i == m_activeTextOverlayIndex)
        {
            CRect leftHandle(textRect.left, textRect.top, textRect.left + 8, textRect.bottom);
            pDC->FillSolidRect(leftHandle, RGB(255, 160, 160));
            CRect rightHandle(textRect.right - 8, textRect.top, textRect.right, textRect.bottom);
            pDC->FillSolidRect(rightHandle, RGB(160, 200, 255));
        }

        CString displayText = overlay.text;
        if (displayText.GetLength() > 10)
        {
            displayText = displayText.Left(7) + _T("...");
        }

        pDC->SetTextColor(RGB(0, 0, 0));
        pDC->SetBkMode(TRANSPARENT);
        pDC->DrawText(displayText, textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

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

    if (m_activeTimelineClipIndex >= 0 && m_activeTimelineClipIndex < (int)m_timelineClips.size())
    {
        const TimelineClip& activeClip = m_timelineClips[m_activeTimelineClipIndex];
        CString infoText;
        infoText.Format(_T("Active Clip - Timeline Pos: %.1fs | Clip Start: %.1fs | Length: %.1fs"),
            activeClip.startTimeOnTimeline, activeClip.clipStartSec, activeClip.clipLengthSec);
        pDC->SetTextColor(RGB(200, 255, 200));
        pDC->TextOut(r.left + 10, r.top + 30, infoText);
    }
    else if (m_activeTextOverlayIndex >= 0 && m_activeTextOverlayIndex < (int)m_textOverlays.size())
    {
        const TextOverlay& activeOverlay = m_textOverlays[m_activeTextOverlayIndex];
        CString infoText;
        infoText.Format(_T("Active Text: \"%s\" | Start: %.1fs | Duration: %.1fs | Position: (%d,%d)"),
            activeOverlay.text, activeOverlay.startSec, activeOverlay.durSec,
            activeOverlay.position.x, activeOverlay.position.y);
        pDC->SetTextColor(RGB(255, 255, 200));
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
        L"MP4 video|*.mp4|All Files|*.*||");
    sfd.m_ofn.lpstrDefExt = L"mp4";
    if (sfd.DoModal() != IDOK) return;

    CString out = sfd.GetPathName();
    if (out.ReverseFind(L'.') < 0)
        out += L".mp4";

    std::vector<TimelineClip> sortedClips = m_timelineClips;
    std::sort(sortedClips.begin(), sortedClips.end(),
        [](const TimelineClip& a, const TimelineClip& b) {
            return a.startTimeOnTimeline < b.startTimeOnTimeline;
        });

    CStringA textFilter = BuildTextOverlayFilter();

    if (sortedClips.size() == 1)
    {
        const TimelineClip& clip = sortedClips[0];
        CStringA inA(clip.path);
        CStringA outA(out);
        CStringA ff;

        if (!textFilter.IsEmpty())
        {
            ff.Format("ffmpeg -y -ss %.3f -i \"%s\" -t %.3f -vf \"%s\" -c:v libx264 -preset veryfast -crf 20 -c:a aac \"%s\"",
                clip.clipStartSec, inA.GetString(), clip.clipLengthSec, textFilter.GetString(), outA.GetString());
        }
        else
        {
            ff.Format("ffmpeg -y -ss %.3f -i \"%s\" -t %.3f -c:v libx264 -preset veryfast -crf 20 -c:a aac \"%s\"",
                clip.clipStartSec, inA.GetString(), clip.clipLengthSec, outA.GetString());
        }

        ExecuteFFmpegCommand(ff.GetString());
    }
    else
    {
        CStringA outA(out);
        CStringA ff;

        ff = "ffmpeg -y";
        for (const auto& clip : sortedClips)
        {
            CStringA pathA(clip.path);
            CStringA input;
            input.Format(" -ss %.3f -t %.3f -i \"%s\"",
                clip.clipStartSec, clip.clipLengthSec, pathA.GetString());
            ff += input;
        }

        CStringA filterComplex = " -filter_complex \"";
        for (int i = 0; i < (int)sortedClips.size(); i++)
        {
            CStringA segment;
            segment.Format("[%d:v:0][%d:a:0]", i, i);
            filterComplex += segment;
        }

        CStringA concatFilter;
        concatFilter.Format("concat=n=%d:v=1:a=1[concat_v][concat_a]", (int)sortedClips.size());
        filterComplex += concatFilter;

        if (!textFilter.IsEmpty())
        {
            filterComplex += ";[concat_v]";
            filterComplex += textFilter;
            filterComplex += "[outv]\"";
            ff += filterComplex;
            ff += " -map \"[outv]\" -map \"[concat_a]\"";
        }
        else
        {
            filterComplex += "\"";
            ff += filterComplex;
            ff += " -map \"[concat_v]\" -map \"[concat_a]\"";
        }

        ff += " -c:v libx264 -preset veryfast -crf 20 -c:a aac";
        CStringA outputPart;
        outputPart.Format(" \"%s\"", outA.GetString());
        ff += outputPart;

        ExecuteFFmpegCommand(ff.GetString());
    }
}

void NWPVideoEditorView::OnRButtonDown(UINT nFlags, CPoint point)
{
    if (!m_rcTimeline.PtInRect(point))
    {
        CView::OnRButtonDown(nFlags, point);
        return;
    }

    int hitClipIndex = HitTestTimelineClip(point);
    if (hitClipIndex >= 0)
    {
        m_contextMenuClipIndex = hitClipIndex;
        m_activeTimelineClipIndex = hitClipIndex;
        InvalidateRect(m_rcTimeline, FALSE);

        CMenu contextMenu;
        contextMenu.CreatePopupMenu();

        CString clipName = m_timelineClips[hitClipIndex].path;
        int lastSlash = clipName.ReverseFind('\\');
        if (lastSlash != -1) clipName = clipName.Mid(lastSlash + 1);

        CString splitText;
        if (m_showSelectionBar && m_hoverClipIndex == hitClipIndex)
        {
            splitText.Format(_T("Split \"%s\" at %.2fs"), clipName, m_hoverTimePosition);
        }
        else
        {
            splitText.Format(_T("Split \"%s\""), clipName);
        }
        contextMenu.AppendMenu(MF_STRING, ID_TIMELINE_SPLIT_CLIP, splitText);
        contextMenu.AppendMenu(MF_SEPARATOR);

        CString removeText;
        removeText.Format(_T("Remove \"%s\" from Timeline"), clipName);
        contextMenu.AppendMenu(MF_STRING, ID_TIMELINE_REMOVE_CLIP, removeText);

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
        CString clipName = m_timelineClips[m_contextMenuClipIndex].path;
        int lastSlash = clipName.ReverseFind('\\');
        if (lastSlash != -1) clipName = clipName.Mid(lastSlash + 1);

        CString message;
        message.Format(_T("Remove \"%s\" from the timeline?"), clipName);

        if (AfxMessageBox(message, MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
            m_timelineClips.erase(m_timelineClips.begin() + m_contextMenuClipIndex);
            if (m_activeTimelineClipIndex == m_contextMenuClipIndex)
            {
                m_activeTimelineClipIndex = -1;
            }
            else if (m_activeTimelineClipIndex > m_contextMenuClipIndex)
            {
                m_activeTimelineClipIndex--;
            }
            RepositionClipsAfterRemoval();
            UpdatePreview();
            InvalidateRect(m_rcTimeline, FALSE);
        }
    }
    m_contextMenuClipIndex = -1;
}

void NWPVideoEditorView::OnTimelineSplitClip()
{
    if (m_contextMenuClipIndex < 0 || m_contextMenuClipIndex >= (int)m_timelineClips.size())
        return;

    TimelineClip& originalClip = m_timelineClips[m_contextMenuClipIndex];

    double splitTime;
    if (m_showSelectionBar && m_hoverClipIndex == m_contextMenuClipIndex)
    {
        splitTime = m_hoverTimePosition;
    }
    else
    {
        splitTime = originalClip.clipStartSec + (originalClip.clipLengthSec / 2.0);
    }

    if (splitTime <= originalClip.clipStartSec ||
        splitTime >= (originalClip.clipStartSec + originalClip.clipLengthSec))
    {
        AfxMessageBox(_T("Invalid split position."));
        return;
    }

    TimelineClip secondClip = originalClip;
    secondClip.startTimeOnTimeline = originalClip.startTimeOnTimeline + (splitTime - originalClip.clipStartSec);
    secondClip.clipStartSec = splitTime;
    secondClip.clipLengthSec = (originalClip.clipStartSec + originalClip.clipLengthSec) - splitTime;

    originalClip.clipLengthSec = splitTime - originalClip.clipStartSec;

    m_timelineClips.insert(m_timelineClips.begin() + m_contextMenuClipIndex + 1, secondClip);

    double adjustmentTime = secondClip.clipLengthSec;
    for (int i = m_contextMenuClipIndex + 2; i < (int)m_timelineClips.size(); i++)
    {
        m_timelineClips[i].startTimeOnTimeline += adjustmentTime;
    }

    m_activeTimelineClipIndex = m_contextMenuClipIndex;

    m_showSelectionBar = false;
    m_hoverClipIndex = -1;
    m_hoverTimePosition = -1.0;

    UpdatePreview();
    InvalidateRect(m_rcTimeline, FALSE);

    m_contextMenuClipIndex = -1;
}

void NWPVideoEditorView::OnPlayPause()
{
    if (m_isPlaying)
    {
        StopPlayback();
    }
    else
    {
        StartPlayback();
    }
}

void NWPVideoEditorView::OnStop()
{
    StopPlayback();
    if (m_activeTimelineClipIndex >= 0 && m_activeTimelineClipIndex < (int)m_timelineClips.size())
    {
        const TimelineClip& activeClip = m_timelineClips[m_activeTimelineClipIndex];
        m_previewTimePosition = activeClip.clipStartSec;
        LoadPreviewFrame(activeClip.path, m_previewTimePosition);
    }
}

void NWPVideoEditorView::StartPlayback()
{
    if (m_activeTimelineClipIndex >= 0 && m_activeTimelineClipIndex < (int)m_timelineClips.size())
    {
        m_isPlaying = true;
        m_playbackStartTime = m_previewTimePosition;
        m_playbackStartTick = GetTickCount();
        m_playbackTimer = SetTimer(1, 100, NULL);
        m_playPauseButton.SetWindowText(_T("Pause"));
    }
}

void NWPVideoEditorView::StopPlayback()
{
    if (m_playbackTimer)
    {
        KillTimer(m_playbackTimer);
        m_playbackTimer = 0;
    }
    m_isPlaying = false;
    m_playPauseButton.SetWindowText(_T("Play"));
}

void NWPVideoEditorView::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1 && m_isPlaying)
    {
        UpdatePlaybackFrame();
    }
    CView::OnTimer(nIDEvent);
}

void NWPVideoEditorView::UpdatePlaybackFrame()
{
    if (m_activeTimelineClipIndex < 0 || m_activeTimelineClipIndex >= (int)m_timelineClips.size())
    {
        StopPlayback();
        return;
    }

    const TimelineClip& activeClip = m_timelineClips[m_activeTimelineClipIndex];
    DWORD currentTick = GetTickCount();
    double elapsed = (currentTick - m_playbackStartTick) / 1000.0;

    double newTime = m_playbackStartTime + elapsed;
    double clipEndTime = activeClip.clipStartSec + activeClip.clipLengthSec;

    if (newTime >= clipEndTime)
    {
        newTime = clipEndTime - 0.1;
        StopPlayback();
    }

    if (newTime != m_previewTimePosition)
    {
        m_previewTimePosition = newTime;
        LoadPreviewFrame(activeClip.path, m_previewTimePosition);
    }
}

double NWPVideoEditorView::GetCurrentClipDuration()
{
    if (m_activeTimelineClipIndex >= 0 && m_activeTimelineClipIndex < (int)m_timelineClips.size())
    {
        return m_timelineClips[m_activeTimelineClipIndex].clipLengthSec;
    }
    return 0.0;
}

void NWPVideoEditorView::RepositionClipsAfterRemoval()
{
    std::sort(m_timelineClips.begin(), m_timelineClips.end(),
        [](const TimelineClip& a, const TimelineClip& b) {
            return a.startTimeOnTimeline < b.startTimeOnTimeline;
        });
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
    UpdatePreview();
    InvalidateRect(m_rcTimeline, FALSE);
}

double NWPVideoEditorView::GetVideoDuration(const CString& filePath)
{
    CStringA pathA(filePath);
    CStringA cmd;
    cmd.Format("ffprobe -v error -select_streams v:0 -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 \"%s\"",
        pathA.GetString());
    std::string wrapper = std::string("cmd /C ") + cmd.GetString();
    std::vector<char> mutableCmd(wrapper.begin(), wrapper.end());
    mutableCmd.push_back('\0');
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
        return 30.0;
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
        return 30.0;
    }
    CloseHandle(hWritePipe);
    char buffer[1024];
    DWORD bytesRead;
    std::string output;
    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        output += buffer;
    }
    WaitForSingleObject(pi.hProcess, 5000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadPipe);
    double duration = 30.0;
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
    double originalDuration = 30.0;
    int imageIndex = -1;
    for (const auto& clip : m_importedClips) {
        if (clip.path == clipPath) {
            originalDuration = clip.durationSec > 0 ? clip.durationSec : 30.0;
            imageIndex = clip.iImage;
            break;
        }
    }
    double newStartTime = 0.0;
    for (const auto& timelineClip : m_timelineClips) {
        double clipEnd = timelineClip.startTimeOnTimeline + timelineClip.clipLengthSec;
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
    m_activeTimelineClipIndex = (int)m_timelineClips.size() - 1;
    UpdatePreview();
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
    int top = r.top + 48;
    int bottom = r.bottom - 56;
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

void NWPVideoEditorView::UpdatePreview()
{
    StopPlayback();
    if (m_activeTimelineClipIndex >= 0 && m_activeTimelineClipIndex < (int)m_timelineClips.size())
    {
        const TimelineClip& activeClip = m_timelineClips[m_activeTimelineClipIndex];
        m_previewTimePosition = activeClip.clipStartSec;
        LoadPreviewFrame(activeClip.path, m_previewTimePosition);
    }
    else
    {
        int sel = m_list.GetNextItem(-1, LVNI_SELECTED);
        if (sel >= 0 && sel < (int)m_importedClips.size())
        {
            m_previewTimePosition = 0.0;
            LoadPreviewFrame(m_importedClips[sel].path, m_previewTimePosition);
        }
        else
        {
            ClearPreview();
        }
    }
}

void NWPVideoEditorView::LoadPreviewFrame(const CString& filePath, double timePosition)
{
    m_currentPreviewPath = filePath;
    if (m_hPreviewBitmap)
    {
        DeleteObject(m_hPreviewBitmap);
        m_hPreviewBitmap = nullptr;
    }
    TCHAR tempPath[MAX_PATH];
    GetTempPath(MAX_PATH, tempPath);
    CString tempImagePath;
    tempImagePath.Format(_T("%stemp_frame_%d.bmp"), tempPath, GetTickCount());
    CStringA pathA(filePath);
    CStringA tempImagePathA(tempImagePath);
    CStringA cmd;
    double seekTime = max(1.0, timePosition);
    cmd.Format("ffmpeg -y -ss %.3f -i \"%s\" -vframes 1 -s 480x360 \"%s\"",
        seekTime, pathA.GetString(), tempImagePathA.GetString());
    std::string wrapper = std::string("cmd /C ") + cmd.GetString() + " >nul 2>&1";
    std::vector<char> mutableCmd(wrapper.begin(), wrapper.end());
    mutableCmd.push_back('\0');
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    BOOL success = CreateProcessA(nullptr, mutableCmd.data(), nullptr, nullptr, FALSE,
        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    if (success)
    {
        WaitForSingleObject(pi.hProcess, 10000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        if (GetFileAttributes(tempImagePath) != INVALID_FILE_ATTRIBUTES)
        {
            LoadBitmapFromFile(tempImagePath);
        }
        DeleteFile(tempImagePath);
    }
    if (!m_hPreviewBitmap)
    {
        CreateTestFrame(filePath, timePosition);
    }
    InvalidateRect(m_rcPreview, FALSE);
}

void NWPVideoEditorView::LoadBitmapFromFile(const CString& imagePath)
{
    m_hPreviewBitmap = (HBITMAP)LoadImage(NULL, imagePath, IMAGE_BITMAP,
        0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
}

void NWPVideoEditorView::CreateTestFrame(const CString& filePath, double timePosition)
{
    CClientDC dc(this);
    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    m_hPreviewBitmap = CreateCompatibleBitmap(dc, 480, 360);
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
    int lastSlash = filename.ReverseFind('\\');
    if (lastSlash != -1) filename = filename.Mid(lastSlash + 1);
    CRect titleRect(20, 150, 460, 190);
    memDC.DrawText(filename, titleRect, DT_CENTER | DT_VCENTER | DT_WORDBREAK);
    CString timeText;
    timeText.Format(_T("Time: %.2fs"), timePosition);
    CRect timeRect(20, 200, 460, 230);
    memDC.DrawText(timeText, timeRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    CString statusText = _T("FFmpeg frame extraction failed");
    CRect statusRect(20, 250, 460, 280);
    memDC.SetTextColor(RGB(255, 200, 100));
    memDC.DrawText(statusText, statusRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    memDC.SelectObject(oldBitmap);
}

void NWPVideoEditorView::CreateBlackFrame()
{
    CClientDC dc(this);
    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    m_hPreviewBitmap = CreateCompatibleBitmap(dc, 480, 360);
    HBITMAP oldBitmap = (HBITMAP)memDC.SelectObject(m_hPreviewBitmap);
    memDC.FillSolidRect(0, 0, 480, 360, RGB(0, 0, 0));
    memDC.SelectObject(oldBitmap);
}

void NWPVideoEditorView::ClearPreview()
{
    StopPlayback();
    if (m_hPreviewBitmap)
    {
        DeleteObject(m_hPreviewBitmap);
        m_hPreviewBitmap = nullptr;
    }
    m_currentPreviewPath.Empty();
    m_previewTimePosition = 0.0;
    InvalidateRect(m_rcPreview, FALSE);
}

void NWPVideoEditorView::ExecuteFFmpegCommand(const char* command)
{
    std::string wrapper = std::string("cmd /C ") + command;
    std::vector<char> mutableCmd(wrapper.begin(), wrapper.end());
    mutableCmd.push_back('\0');

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi{};
    BOOL ok = CreateProcessA(nullptr, mutableCmd.data(), nullptr, nullptr, FALSE,
        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

    if (!ok) {
        AfxMessageBox(L"Failed to start ffmpeg. Ensure it's in PATH.");
        return;
    }

    AfxMessageBox(L"Exporting... This may take a while. Click OK to continue in background.");

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    if (exitCode == 0)
        AfxMessageBox(L"Export finished successfully.");
    else
        AfxMessageBox(L"Export failed. Check that all video files are accessible.");
}

void NWPVideoEditorView::OnFileSave()
{
    OnFileExport();
}

void NWPVideoEditorView::OnEditAddText()
{
    OnAddText();
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
