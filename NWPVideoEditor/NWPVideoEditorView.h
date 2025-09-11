#pragma once
#include <vector>
#include <afxcmn.h>
#include "TextInputDialog.h"

struct ClipItem {
    CString path;
    double  durationSec = 0.0;
    int     iImage = -1;
};

struct TimelineClip {
    CString path;
    double startTimeOnTimeline = 0.0;
    double clipStartSec = 0.0;
    double clipLengthSec = 10.0;
    double originalDuration = 30.0;
    int iImage = -1;
};

struct TextOverlay {
    CString text;
    double startSec;
    double durSec;
    COLORREF color;
    CString fontName;
    int fontSize;
    CPoint position;

    TextOverlay() : startSec(0.0), durSec(5.0), color(RGB(255, 255, 255)),
        fontName(_T("Arial")), fontSize(24), position(50, 50) {
    }
};

struct OverlayItem {
    CString text;
    double startSec;
    double durSec;
};

class CNWPVideoEditorDoc;

class NWPVideoEditorView : public CView
{
protected:
    NWPVideoEditorView();
    DECLARE_DYNCREATE(NWPVideoEditorView)

public:
    CNWPVideoEditorDoc* GetDocument() const;

    CListCtrl   m_list;
    CImageList  m_imgLarge;
    std::vector<ClipItem> m_importedClips;

    CRect       m_rcPreview;
    HBITMAP     m_hPreviewBitmap;
    CString     m_currentPreviewPath;
    double      m_previewTimePosition;

    // Native video size and preview mapping
    int m_videoWidth = 480;
    int m_videoHeight = 360;
    double m_prevScaleX = 1.0;
    double m_prevScaleY = 1.0;
    int m_prevOffX = 0;
    int m_prevOffY = 0;

    std::vector<TimelineClip> m_timelineClips;
    int m_activeTimelineClipIndex = -1;
    CRect   m_rcTimeline;

    std::vector<TextOverlay> m_textOverlays;
    int m_activeTextOverlayIndex = -1;

    std::vector<OverlayItem> m_overlays;

    enum DragState {
        DRAG_NONE,
        DRAG_LEFT_HANDLE,
        DRAG_RIGHT_HANDLE,
        DRAG_CLIP_MOVE,
        DRAG_TEXT_TIMELINE,
        DRAG_TEXT_LEFT_HANDLE,
        DRAG_TEXT_RIGHT_HANDLE,
        DRAG_TEXT_PREVIEW
    };

    DragState m_dragState = DRAG_NONE;
    CPoint m_dragStart{};
    double m_dragStartClipStart = 0.0;
    double m_dragStartClipLength = 0.0;
    double m_dragStartTimelinePos = 0.0;
    double m_dragStartTextStart = 0.0;
    double m_dragStartTextDur = 0.0;
    int m_draggingTextIndex = -1;
    CPoint m_textDragStart{};

    int m_nDragIndex = -1;
    BOOL m_bDragging = FALSE;
    CImageList* m_pDragImage = nullptr;

    double m_timelineDurationSec = 60.0;
    double m_timelineScrollOffset = 0.0;
    int m_contextMenuClipIndex = -1;

    double m_hoverTimePosition = -1.0;
    int m_hoverClipIndex = -1;
    bool m_showSelectionBar = false;

    CButton m_playPauseButton;
    CButton m_stopButton;
    CButton m_addTextButton;
    bool m_isPlaying = false;
    UINT_PTR m_playbackTimer = 0;
    double m_playbackStartTime = 0.0;
    DWORD m_playbackStartTick = 0;

public:
    virtual void OnDraw(CDC* pDC);
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

protected:
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
    afx_msg void OnFileSave();
    afx_msg void OnLButtonDown(UINT nFlags, CPoint pt);
    afx_msg void OnMouseMove(UINT nFlags, CPoint pt);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint pt);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint pt);
    afx_msg void OnListDblClk(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnListItemActivate(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnListBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnEditAddText();
    afx_msg void OnAddText();
    afx_msg void OnDeleteTextOverlay();
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnTimelineRemoveClip();
    afx_msg void OnTimelineSplitClip();
    afx_msg void OnPlayPause();
    afx_msg void OnStop();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    DECLARE_MESSAGE_MAP()

private:
    int  AddShellIconForFile(const CString& path);
    void Layout(int cx, int cy);
    void DrawTimeline(CDC* pDC);
    void DrawPreviewFrame(CDC* pDC);
    void DrawTextOverlays(CDC* pDC);
    void UpdatePreview();
    void LoadPreviewFrame(const CString& filePath, double timePosition);
    void LoadBitmapFromFile(const CString& imagePath);
    void CreateTestFrame(const CString& filePath, double timePosition);
    void CreateBlackFrame();
    void ClearPreview();
    void SetActiveClipFromSelection();
    int  HitTestTimelineHandle(CPoint pt) const;
    int  HitTestTimelineClip(CPoint pt) const;
    int  HitTestTimelineTextOverlay(CPoint pt) const;
    int  HitTestTextOverlayHandles(CPoint pt) const;
    int  HitTestTextOverlayInPreview(CPoint pt) const;
    CString BuildTextOverlayFilter() const;
    double TimelineXToSeconds(int x) const;
    int SecondsToTimelineX(double seconds) const;
    double GetVideoDurationAndSize(const CString& filePath, int& outW, int& outH);
    void RepositionClipsAfterRemoval();
    BOOL IsOverTimeline(CPoint screenPt);
    void AddClipToTimeline(const CString& clipPath);
    void StartPlayback();
    void StopPlayback();
    void UpdatePlaybackFrame();
    double GetCurrentClipDuration();
    void ExecuteFFmpegCommand(const CString& command);
    static BOOL RunProcessAndWait(const CString& cmdLine, DWORD waitMS, DWORD* pExit = nullptr);

};

#ifndef _DEBUG
inline CNWPVideoEditorDoc* NWPVideoEditorView::GetDocument() const
{
    return reinterpret_cast<CNWPVideoEditorDoc*>(m_pDocument);
}
#endif
