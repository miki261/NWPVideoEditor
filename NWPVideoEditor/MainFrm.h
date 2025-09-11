#pragma once

class CMainFrame : public CFrameWndEx
{
protected:
    CMainFrame() noexcept;
    DECLARE_DYNCREATE(CMainFrame)

public:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual BOOL LoadFrame(
        UINT nIDResource,
        DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE,
        CWnd* pParentWnd = nullptr,
        CCreateContext* pContext = nullptr);
    virtual ~CMainFrame();

#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:
    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
    DECLARE_MESSAGE_MAP()
};
