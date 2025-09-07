#include "pch.h"
#include "TextInputDialog.h"

BEGIN_MESSAGE_MAP(CTextInputDialog, CWnd)
    ON_WM_CREATE()
    ON_WM_PAINT()
    ON_BN_CLICKED(IDOK, OnOK)
    ON_BN_CLICKED(IDCANCEL, OnCancel)
    ON_WM_CLOSE()
END_MESSAGE_MAP()

CTextInputDialog::CTextInputDialog()
{
    m_modalResult = false;
    m_result = IDCANCEL;
}

CTextInputDialog::~CTextInputDialog()
{
}

INT_PTR CTextInputDialog::DoModal(CWnd* pParent)
{
    CString className = AfxRegisterWndClass(
        CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW,
        ::LoadCursor(NULL, IDC_ARROW),
        (HBRUSH)(COLOR_BTNFACE + 1),
        NULL);

    if (!CreateEx(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        className,
        _T("Add Text Overlay"),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        0, 0, 350, 120,
        pParent ? pParent->GetSafeHwnd() : NULL,
        NULL))
    {
        return IDCANCEL;
    }

    CenterWindow(pParent);

    if (pParent)
        pParent->EnableWindow(FALSE);

    MSG msg;
    m_modalResult = false;

    while (!m_modalResult && GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN)
        {
            OnOK();
            continue;
        }
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
        {
            OnCancel();
            continue;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (pParent)
        pParent->EnableWindow(TRUE);

    DestroyWindow();
    return m_result;
}

int CTextInputDialog::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    m_font.CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("Segoe UI"));

    m_edit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
        CRect(15, 25, 320, 50), this, 1001);
    m_edit.SetFont(&m_font);
    m_edit.SetWindowText(_T("Your text here"));
    m_edit.SetSel(0, -1);
    m_edit.SetFocus();

    m_btnOK.Create(_T("Add Text"), WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
        CRect(170, 60, 240, 85), this, IDOK);
    m_btnOK.SetFont(&m_font);

    m_btnCancel.Create(_T("Cancel"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        CRect(250, 60, 320, 85), this, IDCANCEL);
    m_btnCancel.SetFont(&m_font);

    return 0;
}

void CTextInputDialog::OnPaint()
{
    CPaintDC dc(this);

    CFont* oldFont = dc.SelectObject(&m_font);
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(RGB(0, 0, 0));
    dc.TextOut(15, 8, _T("Enter text:"));
    dc.SelectObject(oldFont);
}

void CTextInputDialog::OnOK()
{
    m_edit.GetWindowText(m_text);

    if (m_text.IsEmpty() || m_text == _T("Your text here"))
    {
        MessageBox(_T("Please enter some text."), _T("Text Required"), MB_OK | MB_ICONINFORMATION);
        m_edit.SetFocus();
        m_edit.SetSel(0, -1);
        return;
    }

    m_result = IDOK;
    m_modalResult = true;
}

void CTextInputDialog::OnCancel()
{
    m_result = IDCANCEL;
    m_modalResult = true;
}

void CTextInputDialog::OnClose()
{
    OnCancel();
}
