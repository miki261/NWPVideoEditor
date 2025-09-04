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
    ci.durationSec = 30.0; // Default to 30 seconds
    m_timeline.push_back(ci);

    CString name = dlg.GetFileName();
    int img = (ci.iImage >= 0) ? ci.iImage : 0;
    int row = m_list.InsertItem(m_list.GetItemCount(), name, img);
    m_list.SetItemState(row, LVIS_SELECTED, LVIS_SELECTED);
}

void NWPVideoEditorView::SetActiveClipFromSelection()
{
    int sel = m_list.GetNextItem(-1, LVNI_SELECTED);
    if (sel < 0 || sel >= (int)m_timeline.size()) return;
    m_activeClipPath = m_timeline[sel].path;
    if (m_activeClipLenSec < 1.0) m_activeClipLenSec = 10.0;
    InvalidateRect(m_rcTimeline, FALSE);
}

void NWPVideoEditorView::AddClipToTimeline(const CString& clipPath)
{
    // Find the clip's original duration
    m_originalClipDuration = 30.0; // default
    for (const auto& clip : m_timeline) {
        if (clip.path == clipPath) {
            m_originalClipDuration = clip.durationSec > 0 ? clip.durationSec : 30.0;
            break;
        }
    }

    // Reset all states
    if (GetCapture() == this) ReleaseCapture();
    m_dragState = DRAG_NONE;
    m_bDragging = FALSE;

    m_activeClipPath = clipPath;
    m_activeClipLenSec = min(10.0, m_originalClipDuration);
    m_activeClipStartSec = 0.0;
    InvalidateRect(m_rcTimeline, FALSE);
}

void NWPVideoEditorView::OnListDblClk(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMITEMACTIVATE* pNMItemActivate = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    int i = pNMItemActivate->iItem;

    if (i >= 0 && i < (int)m_timeline.size())
    {
        AddClipToTimeline(m_timeline[i].path);
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

    if (m_nDragIndex >= 0 && m_nDragIndex < (int)m_timeline.size())
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

int NWPVideoEditorView::HitTestTimelineHandle(CPoint pt) const
{
    if (m_activeClipPath.IsEmpty()) return 0;
    const CRect& r = m_rcTimeline;
    int left = r.left + 12;
    int right = r.right - 12;
    int top = r.top + 48;

    auto XForSec = [&](double s)->int {
        int span = (int)((right - left) * 0.70);
        return left + (int)(span * (s / max(1.0, m_originalClipDuration)));
        };

    int clipStartX = XForSec(m_activeClipStartSec);
    int clipEndX = XForSec(m_activeClipStartSec + m_activeClipLenSec);

    CRect clipR(clipStartX, top, clipEndX, r.bottom - 36);

    // Left resize handle
    CRect leftHandleR(clipR.left, clipR.top, clipR.left + 8, clipR.bottom);
    if (leftHandleR.PtInRect(pt)) return 3;    // Left handle

    // Right resize handle  
    CRect rightHandleR(clipR.right - 8, clipR.top, clipR.right, clipR.bottom);
    if (rightHandleR.PtInRect(pt)) return 2;   // Right handle

    if (clipR.PtInRect(pt)) return 1;          // Inside clip
    return 0;
}

void NWPVideoEditorView::OnLButtonDown(UINT nFlags, CPoint pt)
{
    if (!m_rcTimeline.PtInRect(pt) || m_activeClipPath.IsEmpty())
        return;

    m_dragState = DRAG_NONE;
    int hit = HitTestTimelineHandle(pt);

    if (hit == 3) // Left handle
    {
        m_dragState = DRAG_LEFT_HANDLE;
        m_dragStart = pt;
        m_dragStartClipStart = m_activeClipStartSec;
        m_dragStartClipLength = m_activeClipLenSec;
        SetCapture();
    }
    else if (hit == 2) // Right handle
    {
        m_dragState = DRAG_RIGHT_HANDLE;
        m_dragStart = pt;
        m_dragStartClipStart = m_activeClipStartSec;
        m_dragStartClipLength = m_activeClipLenSec;
        SetCapture();
    }
}

void NWPVideoEditorView::OnMouseMove(UINT nFlags, CPoint point)
{
    // Handle clip drag-and-drop
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

    // Handle timeline handle dragging
    if (m_dragState != DRAG_NONE && GetCapture() == this)
    {
        const CRect& r = m_rcTimeline;
        int timelineWidth = (int)((r.Width() - 24) * 0.70);
        if (timelineWidth <= 0) return;

        int dx = point.x - m_dragStart.x;
        double pixelsPerSecond = timelineWidth / m_originalClipDuration;
        double deltaSeconds = dx / pixelsPerSecond;

        if (m_dragState == DRAG_LEFT_HANDLE)
        {
            double newStart = m_dragStartClipStart + deltaSeconds;
            newStart = max(0.0, min(m_originalClipDuration - 1.0, newStart));

            double endTime = m_dragStartClipStart + m_dragStartClipLength;
            m_activeClipStartSec = newStart;
            m_activeClipLenSec = max(1.0, endTime - newStart);
        }
        else if (m_dragState == DRAG_RIGHT_HANDLE)
        {
            double newLength = m_dragStartClipLength + deltaSeconds;
            double maxLength = m_originalClipDuration - m_activeClipStartSec;
            m_activeClipLenSec = max(1.0, min(maxLength, newLength));
        }

        InvalidateRect(m_rcTimeline, FALSE);
        return;
    }

    // Cursor feedback when not dragging
    if (m_rcTimeline.PtInRect(point) && m_dragState == DRAG_NONE)
    {
        int hit = HitTestTimelineHandle(point);
        if (hit == 2 || hit == 3)
            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
        else if (hit == 1)
            SetCursor(LoadCursor(NULL, IDC_HAND));
        else
            SetCursor(LoadCursor(NULL, IDC_ARROW));
    }
}

void NWPVideoEditorView::OnLButtonUp(UINT nFlags, CPoint point)
{
    // Handle drag-drop completion
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
        if (IsOverTimeline(ptScreen) && m_nDragIndex >= 0)
        {
            AddClipToTimeline(m_timeline[m_nDragIndex].path);
        }

        m_nDragIndex = -1;
        return;
    }

    // Handle timeline dragging completion
    if (m_dragState != DRAG_NONE)
    {
        if (GetCapture() == this) ReleaseCapture();
        m_dragState = DRAG_NONE;
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

    if (m_activeClipPath.IsEmpty()) {
        pDC->TextOut(r.left + 10, r.top + 30, _T("Double-click or drag a clip above to place it on the timeline."));
        return;
    }

    int left = r.left + 12;
    int right = r.right - 12;
    int top = r.top + 48;

    auto XForSec = [&](double s)->int {
        int span = (int)((right - left) * 0.70);
        return left + (int)(span * (s / max(1.0, m_originalClipDuration)));
        };

    int clipStartX = XForSec(m_activeClipStartSec);
    int clipEndX = XForSec(m_activeClipStartSec + m_activeClipLenSec);

    CRect clipR(clipStartX, top, clipEndX, r.bottom - 36);
    pDC->FillSolidRect(clipR, RGB(70, 120, 220));
    pDC->FrameRect(clipR, &CBrush(RGB(30, 60, 140)));

    // Left handle
    CRect leftHandleR(clipR.left, clipR.top, clipR.left + 8, clipR.bottom);
    pDC->FillSolidRect(leftHandleR, RGB(255, 160, 160));

    // Right handle
    CRect rightHandleR(clipR.right - 8, clipR.top, clipR.right, clipR.bottom);
    pDC->FillSolidRect(rightHandleR, RGB(160, 200, 255));

    // Text
    CString durText;
    durText.Format(_T("Start: %.1fs | Length: %.1fs"), m_activeClipStartSec, m_activeClipLenSec);
    CRect textR = clipR;
    textR.DeflateRect(12, 2);
    pDC->SetBkMode(TRANSPARENT);
    pDC->SetTextColor(RGB(255, 255, 255));
    pDC->DrawText(durText, textR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // Overlays
    for (const auto& ov : m_overlays) {
        int x1 = XForSec(ov.startSec);
        int x2 = XForSec(ov.startSec + ov.durSec);
        if (x2 > x1) {
            CRect ovR(x1, r.bottom - 24, x2, r.bottom - 4);
            pDC->FillSolidRect(ovR, RGB(255, 200, 100));
            pDC->FrameRect(ovR, &CBrush(RGB(200, 160, 80)));
            pDC->SetTextColor(RGB(0, 0, 0));
            pDC->DrawText(ov.text, ovR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    }
}

void NWPVideoEditorView::OnFileExport()
{
    if (m_activeClipPath.IsEmpty()) {
        AfxMessageBox(L"Place a clip on the timeline first.");
        return;
    }

    CFileDialog sfd(FALSE, L"mp4", L"output.mp4", OFN_OVERWRITEPROMPT,
        L"MP4 video|*.mp4||");
    if (sfd.DoModal() != IDOK) return;

    CString out = sfd.GetPathName();

    CStringA inA(m_activeClipPath);
    CStringA outA(out);
    CStringA ff;
    ff.Format("ffmpeg -y -ss %.2f -i \"%s\" -t %.2f -c:v libx264 -preset veryfast -crf 20 -c:a aac \"%s\"",
        m_activeClipStartSec, inA.GetString(), m_activeClipLenSec, outA.GetString());

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
