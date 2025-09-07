#pragma once
#include <afxwin.h>

class CTextInputDialog : public CWnd
{
public:
    CTextInputDialog();
    virtual ~CTextInputDialog();

    INT_PTR DoModal(CWnd* pParent = nullptr);
    CString GetText() const { return m_text; }

protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnPaint();
    afx_msg void OnOK();
    afx_msg void OnCancel();
    afx_msg void OnClose();

    DECLARE_MESSAGE_MAP()

private:
    CEdit m_edit;
    CButton m_btnOK;
    CButton m_btnCancel;
    CString m_text;
    CFont m_font;
    bool m_modalResult;
    INT_PTR m_result;
};
