#pragma once
#include <vector>

struct ClipItem {
    CString path;
    double  durationSec = 0.0;
    int     iImage = -1;
};

class CNWPVideoEditorDoc; // Forward declaration

class NWPVideoEditorView : public CView
{
protected:
    NWPVideoEditorView();
    DECLARE_DYNCREATE(NWPVideoEditorView)

public:
    // FIXED - Add GetDocument declaration
    CNWPVideoEditorDoc* GetDocument() const;

    // Media grid
    CListCtrl   m_list;
    CImageList  m_imgLarge;
    std::vector<ClipItem> m_timeline; // imported clips

    // Timeline model
    CString m_activeClipPath;
    double  m_activeClipLenSec = 10.0;
    CRect   m_rcTimeline;

    struct OverlayItem { CString text; double startSec; double durSec; };
    std::vector<OverlayItem> m_overlays;

    bool   m_draggingLength = false;
    double m_dragStartSec = 0.0;
    CPoint m_dragStart{};

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

    // Grid activation - timeline
    afx_msg void OnListDblClk(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnListItemActivate(NMHDR* pNMHDR, LRESULT* pResult);

    // Add Text command
    afx_msg void OnEditAddText();

    DECLARE_MESSAGE_MAP()

private:
    int  AddShellIconForFile(const CString& path);
    void Layout(int cx, int cy);
    void DrawTimeline(CDC* pDC);
    void SetActiveClipFromSelection();
    int  HitTestTimelineHandle(CPoint pt) const;
};

// FIXED - GetDocument implementation for Release builds
#ifndef _DEBUG  // debug version in NWPVideoEditorView.cpp
inline CNWPVideoEditorDoc* NWPVideoEditorView::GetDocument() const
{
    return reinterpret_cast<CNWPVideoEditorDoc*>(m_pDocument);
}
#endif
