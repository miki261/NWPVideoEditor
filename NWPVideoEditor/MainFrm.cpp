// MainFrm.cpp : implementation of the CMainFrame class
//
#include "pch.h"
#include "NWPVideoEditor.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
    ON_WM_CREATE()
    ON_COMMAND(ID_VIEW_CUSTOMIZE, &CMainFrame::OnViewCustomize)
    ON_REGISTERED_MESSAGE(AFX_WM_CREATETOOLBAR, &CMainFrame::OnToolbarCreateNew)
    ON_COMMAND_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CMainFrame::OnApplicationLook)
    ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CMainFrame::OnUpdateApplicationLook)
    ON_WM_SETTINGCHANGE()
END_MESSAGE_MAP()

static UINT indicators[] =
{
    ID_SEPARATOR, // status line indicator
};

CMainFrame::CMainFrame() noexcept {}
CMainFrame::~CMainFrame() {}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
        return -1;

    // Menu bar
    if (!m_wndMenuBar.Create(this))
        return -1;
    m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

    // Tool bar
    if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER |
        CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
        !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
        return -1;

    // Status bar
    if (!m_wndStatusBar.Create(this))
        return -1;
    m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));

    // Enable docking
    EnableDocking(CBRS_ALIGN_ANY);
    DockPane(&m_wndMenuBar);
    DockPane(&m_wndToolBar);

    // File/Class/Output/Properties
    if (!CreateDockingWindows())
        return -1;

    // Set icons for docking windows
    SetDockingWindowIcons(TRUE);

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

BOOL CMainFrame::CreateDockingWindows()
{
    CString strOutputWnd, strPropertiesWnd, strFileView, strClassView;
    strOutputWnd.LoadString(IDS_OUTPUT_WND);
    strPropertiesWnd.LoadString(IDS_PROPERTIES_WND);
    strFileView.LoadString(IDS_FILE_VIEW);
    strClassView.LoadString(IDS_CLASS_VIEW);

    if (!m_wndFileView.Create(strFileView, this, CRect(0, 0, 200, 300), TRUE, ID_VIEW_FILEVIEW,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT))
        return FALSE;

    if (!m_wndClassView.Create(strClassView, this, CRect(0, 0, 200, 300), TRUE, ID_VIEW_CLASSVIEW,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT))
        return FALSE;

    if (!m_wndOutput.Create(strOutputWnd, this, CRect(0, 0, 100, 100), TRUE, ID_VIEW_OUTPUTWND,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_BOTTOM))
        return FALSE;

    if (!m_wndProperties.Create(strPropertiesWnd, this, CRect(0, 0, 200, 300), TRUE, ID_VIEW_PROPERTIESWND,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT))
        return FALSE;

    m_wndFileView.EnableDocking(CBRS_ALIGN_ANY);
    m_wndClassView.EnableDocking(CBRS_ALIGN_ANY);
    m_wndOutput.EnableDocking(CBRS_ALIGN_ANY);
    m_wndProperties.EnableDocking(CBRS_ALIGN_ANY);

    DockPane(&m_wndFileView);
    DockPane(&m_wndClassView);
    DockPane(&m_wndOutput);
    DockPane(&m_wndProperties);

    return TRUE;
}

void CMainFrame::SetDockingWindowIcons(BOOL /*bHiColorIcons*/)
{
}


void CMainFrame::OnViewCustomize()
{
    CMFCToolBarsCustomizeDialog* pDlg = new CMFCToolBarsCustomizeDialog(this, TRUE);
    pDlg->EnableUserDefinedToolbars();
    pDlg->Create();
}

LRESULT CMainFrame::OnToolbarCreateNew(WPARAM, LPARAM)
{
    return 0L;
}

void CMainFrame::OnApplicationLook(UINT)
{
    // default
}

void CMainFrame::OnUpdateApplicationLook(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(FALSE);
}

void CMainFrame::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
    CFrameWndEx::OnSettingChange(uFlags, lpszSection);
    m_wndMenuBar.AdjustLayout();
    m_wndToolBar.AdjustLayout();
}

#ifdef _DEBUG
void CMainFrame::AssertValid() const { CFrameWndEx::AssertValid(); }
void CMainFrame::Dump(CDumpContext& dc) const { CFrameWndEx::Dump(dc); }
#endif
