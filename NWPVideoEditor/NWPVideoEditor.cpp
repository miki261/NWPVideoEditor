#include "pch.h"
#include "framework.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "NWPVideoEditor.h"
#include "MainFrm.h"

#include "NWPVideoEditorDoc.h"
#include "NWPVideoEditorView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CNWPVideoEditorApp, CWinAppEx)
    ON_COMMAND(ID_APP_ABOUT, &CNWPVideoEditorApp::OnAppAbout)
    ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
END_MESSAGE_MAP()

CNWPVideoEditorApp::CNWPVideoEditorApp() noexcept
{
    m_bHiColorIcons = TRUE;
    m_nAppLook = 0;
    m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_ALL_ASPECTS;
#ifdef _MANAGED
    System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif
    SetAppID(_T("NWPVideoEditor.AppID.NoVersion"));
}

CNWPVideoEditorApp theApp;

BOOL CNWPVideoEditorApp::InitInstance()
{
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinAppEx::InitInstance();

    if (!AfxOleInit())
    {
        AfxMessageBox(IDP_OLE_INIT_FAILED);
        return FALSE;
    }

    AfxEnableControlContainer();

    EnableTaskbarInteraction(FALSE);

    SetRegistryKey(_T("Local AppWizard-Generated Applications"));
    LoadStdProfileSettings(4);

    InitContextMenuManager();
    InitKeyboardManager();
    InitTooltipManager();
    CMFCToolTipInfo ttParams;
    ttParams.m_bVislManagerTheme = TRUE;
    theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
        RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

    CSingleDocTemplate* pDocTemplate;
    pDocTemplate = new CSingleDocTemplate(
        IDR_MAINFRAME,
        RUNTIME_CLASS(CNWPVideoEditorDoc),
        RUNTIME_CLASS(CMainFrame),
        RUNTIME_CLASS(NWPVideoEditorView));
    if (!pDocTemplate)
        return FALSE;
    AddDocTemplate(pDocTemplate);

    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    if (!ProcessShellCommand(cmdInfo))
        return FALSE;

    m_pMainWnd->ShowWindow(SW_SHOW);
    m_pMainWnd->UpdateWindow();
    return TRUE;
}

int CNWPVideoEditorApp::ExitInstance()
{
    AfxOleTerm(FALSE);
    return CWinAppEx::ExitInstance();
}

class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg() noexcept;
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ABOUTBOX };
#endif
protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() noexcept : CDialogEx(IDD_ABOUTBOX) {}
void CAboutDlg::DoDataExchange(CDataExchange* pDX) { CDialogEx::DoDataExchange(pDX); }
BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

void CNWPVideoEditorApp::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
}

void CNWPVideoEditorApp::PreLoadState()
{
    BOOL bNameValid;
    CString strName;
    bNameValid = strName.LoadString(IDS_EDIT_MENU);
    ASSERT(bNameValid);
    GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EDIT);
    bNameValid = strName.LoadString(IDS_EXPLORER);
    ASSERT(bNameValid);
    GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EXPLORER);
}

void CNWPVideoEditorApp::LoadCustomState() {}
void CNWPVideoEditorApp::SaveCustomState() {}
