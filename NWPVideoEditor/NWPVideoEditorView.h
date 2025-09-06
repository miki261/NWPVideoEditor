#pragma once
#include <vector>
#include "afxcmn.h"

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

    std::vector<TimelineClip> m_timelineClips;
    int m_activeTimelineClipIndex = -1;
    CRect   m_rcTimeline;

    struct OverlayItem { CString text; double startSec; double durSec; };
    std::vector<OverlayItem> m_overlays;

    enum DragState { DRAG_NONE, DRAG_LEFT_HANDLE, DRAG_RIGHT_HANDLE, DRAG_CLIP_MOVE };
    DragState m_dragState = DRAG_NONE;
    CPoint m_dragStart{};
    double m_dragStartClipStart = 0.0;
    double m_dragStartClipLength = 0.0;
    double m_dragStartTimelinePos = 0.0;

    int m_nDragIndex = -1;
    BOOL m_bDragging = FALSE;
    CImageList* m_pDragImage = nullptr;

    double m_timelineDurationSec = 60.0;
    double m_timelineScrollOffset = 0.0;
    int m_contextMenuClipIndex = -1;

    double m_hoverTimePosition = -1.0;
    int m_hoverClipIndex = -1;
    bool m_showSelectionBar = false;

    // Video playback controls
    CButton m_playPauseButton;
    CButton m_stopButton;
    bool m_isPlaying = false;
    UINT_PTR m_playbackTimer = 0;
    double m_playbackStartTime = 0.0;
    DWORD m_playbackStartTick = 0;

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
    afx_msg void OnFileSave();
    afx_msg void OnLButtonDown(UINT nFlags, CPoint pt);
    afx_msg void OnMouseMove(UINT nFlags, CPoint pt);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint pt);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint pt);
    afx_msg void OnListDblClk(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnListItemActivate(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnListBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnEditAddText();
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
    void UpdatePreview();
    void LoadPreviewFrame(const CString& filePath, double timePosition);
    void LoadBitmapFromFile(const CString& imagePath);
    void CreateTestFrame(const CString& filePath, double timePosition);
    void CreateBlackFrame();
    void ClearPreview();
    void SetActiveClipFromSelection();
    int  HitTestTimelineHandle(CPoint pt) const;
    int  HitTestTimelineClip(CPoint pt) const;
    double TimelineXToSeconds(int x) const;
    int SecondsToTimelineX(double seconds) const;
    double GetVideoDuration(const CString& filePath);
    void RepositionClipsAfterRemoval();
    BOOL IsOverTimeline(CPoint screenPt);
    void AddClipToTimeline(const CString& clipPath);
    void StartPlayback();
    void StopPlayback();
    void UpdatePlaybackFrame();
    double GetCurrentClipDuration();
    void ExecuteFFmpegCommand(const char* command);
};

#ifndef _DEBUG
inline CNWPVideoEditorDoc* NWPVideoEditorView::GetDocument() const
{
    return reinterpret_cast<CNWPVideoEditorDoc*>(m_pDocument);
}
#endif
