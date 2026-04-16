#include "pch.h"
#include "TextInputDialog.h"
#include "Resource.h"

IMPLEMENT_DYNAMIC(CTextInputDialog, CDialogEx)

CTextInputDialog::CTextInputDialog(CWnd* pParent)
    : CDialogEx(IDD_TEXT_INPUT_DIALOG, pParent)
{
}

CTextInputDialog::~CTextInputDialog() {}

void CTextInputDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_TEXT_INPUT_EDIT, m_editCtrl);
}

BOOL CTextInputDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    CString hint;
    hint.LoadString(IDS_YOUR_TEXT_HERE);
    m_editCtrl.SetWindowText(hint);
    m_editCtrl.SetSel(0, -1);
    m_editCtrl.SetFocus();

    return FALSE; // FALSE because we set focus manually
}

void CTextInputDialog::OnOK()
{
    m_editCtrl.GetWindowText(m_text);
    m_text.Trim();

    if (m_text.IsEmpty()) {
        CString msg;
        msg.LoadString(IDS_TEXT_REQUIRED);
        CString title;
        title.LoadString(IDS_PLEASE_ENTER_TEXT);
        MessageBox(msg, title, MB_OK | MB_ICONWARNING);
        m_editCtrl.SetFocus();
        return; // Don't close
    }

    CDialogEx::OnOK();
}

BEGIN_MESSAGE_MAP(CTextInputDialog, CDialogEx)
END_MESSAGE_MAP()