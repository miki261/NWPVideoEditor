#pragma once
#include <vector>
#include <gdiplus.h>
#include "afxcmn.h"

struct ClipItem {
    CString path;
    double  durationSec = 0.0;
    int     iImage = -1;
};

// New structure for timeline clips
struct TimelineClip {
    CString path;
    double startTimeOnTimeline = 0.0;  // Position on the timeline
    double clipStartSec = 0.0;         // Start position within the source clip
    double clipLengthSec = 10.0;       // Length of the clip segment
    double originalDuration = 30.0;     // Original duration of the source clip
    int iImage = -1;
};

class CNWPVideoEditorDoc; // Forward declaration

class NWPVideoEditorView : public CView
{
protected:
    NWPVideoEditorView();
    DECLARE_DYNCREATE(NWPVideoEditorView)

public:
    CNWPVideoEditorDoc* GetDocument() const;

    // Media grid
    CListCtrl   m_list;
    CImageList  m_imgLarge;
    std::vector<ClipItem> m_importedClips; // Renamed from m_timeline

    // Preview window
    CRect       m_rcPreview;            // Preview rectangle
    HBITMAP     m_hPreviewBitmap;       // Current preview frame
    CString     m_currentPreviewPath;   // Currently previewed file
    double      m_previewTimePosition;  // Current time position in preview

    // Timeline model - changed to support multiple clips
    std::vector<TimelineClip> m_timelineClips;
    int m_activeTimelineClipIndex = -1; // Index of currently selected timeline clip
    CRect   m_rcTimeline;

    struct OverlayItem { CString text; double startSec; double durSec; };
    std::vector<OverlayItem> m_overlays;

    // Dragging states
    enum DragState { DRAG_NONE, DRAG_LEFT_HANDLE, DRAG_RIGHT_HANDLE, DRAG_CLIP_MOVE };
    DragState m_dragState = DRAG_NONE;
    CPoint m_dragStart{};
    double m_dragStartClipStart = 0.0;
    double m_dragStartClipLength = 0.0;
    double m_dragStartTimelinePos = 0.0;

    // Drag-drop members for clip list
    int m_nDragIndex = -1;
    BOOL m_bDragging = FALSE;
    CImageList* m_pDragImage = nullptr;

    // Timeline settings
    double m_timelineDurationSec = 60.0; // Total timeline duration
    double m_timelineScrollOffset = 0.0;
    int m_contextMenuClipIndex = -1;  // tracks which clip was right-clicked

public:
    virtual void OnDraw(CDC* pDC);
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

protected:
    virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
    virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
    virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

public:
    virtual ~NWPVideoEditorView();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:
    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnFileImport();
    afx_msg void OnFileExport();
    afx_msg void OnLButtonDown(UINT nFlags, CPoint pt);
    afx_msg void OnMouseMove(UINT nFlags, CPoint pt);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint pt);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint pt);
    // List control event handlers
    afx_msg void OnListDblClk(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnListItemActivate(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnListBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnEditAddText();
    afx_msg void OnTimelineRemoveClip();
    DECLARE_MESSAGE_MAP()

private:
    int  AddShellIconForFile(const CString& path);
    void Layout(int cx, int cy);
    void DrawTimeline(CDC* pDC);
    void DrawPreviewFrame(CDC* pDC);
    void UpdatePreview();
    void LoadPreviewFrame(const CString& filePath, double timePosition);
    void LoadImageFromFile(const CString& imagePath);
    void LoadBitmapFromFile(const CString& imagePath);
    void CreateBlackFrame();
    void ClearPreview();
    void SetActiveClipFromSelection();
    int  HitTestTimelineHandle(CPoint pt) const;
    int  HitTestTimelineClip(CPoint pt) const;
    double TimelineXToSeconds(int x) const;
    int SecondsToTimelineX(double seconds) const;
    double GetVideoDuration(const CString& filePath);
    void RepositionClipsAfterRemoval();
    // Helper functions for drag-drop
    BOOL IsOverTimeline(CPoint screenPt);
    void AddClipToTimeline(const CString& clipPath);
};

#ifndef _DEBUG  // debug version in NWPVideoEditorView.cpp
inline CNWPVideoEditorDoc* NWPVideoEditorView::GetDocument() const
{
    return reinterpret_cast<CNWPVideoEditorDoc*>(m_pDocument);
}
#endif
