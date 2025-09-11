#pragma once

class CNWPVideoEditorDoc : public CDocument
{
protected:
    CNWPVideoEditorDoc() noexcept;
    DECLARE_DYNCREATE(CNWPVideoEditorDoc)

public:
    virtual BOOL OnNewDocument();
    virtual void Serialize(CArchive& ar);

public:
    virtual ~CNWPVideoEditorDoc();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:
    DECLARE_MESSAGE_MAP()
};
