#include "pch.h"
#include "NWPVideoEditor.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
    ON_WM_CREATE()
    ON_WM_SETTINGCHANGE()
END_MESSAGE_MAP()

CMainFrame::CMainFrame() noexcept {}
CMainFrame::~CMainFrame() {}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
        return -1;

    EnableDocking(CBRS_ALIGN_ANY);

    return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!CFrameWndEx::PreCreateWindow(cs))
        return FALSE;
    return TRUE;
}

BOOL CMainFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext)
{
    return CFrameWndEx::LoadFrame(nIDResource, dwDefaultStyle, pParentWnd, pContext);
}

void CMainFrame::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
    CFrameWndEx::OnSettingChange(uFlags, lpszSection);
}

#ifdef _DEBUG
void CMainFrame::AssertValid() const { CFrameWndEx::AssertValid(); }
void CMainFrame::Dump(CDumpContext& dc) const { CFrameWndEx::Dump(dc); }
#endif
