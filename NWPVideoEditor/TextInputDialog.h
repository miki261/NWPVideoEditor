#pragma once
#include <afxdialogex.h>
#include "resource.h"

class CTextInputDialog : public CDialogEx
{
    DECLARE_DYNAMIC(CTextInputDialog)

public:
    CTextInputDialog(CWnd* pParent = nullptr);
    virtual ~CTextInputDialog();

    CString GetText() const { return m_text; }

    enum { IDD = IDD_TEXT_INPUT_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();

    DECLARE_MESSAGE_MAP()

private:
    CString  m_text;
    CEdit    m_editCtrl;
};