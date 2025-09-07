
// NWPVideoEditorDoc.h : interface of the CNWPVideoEditorDoc class
//


#pragma once


class CNWPVideoEditorDoc : public CDocument
{
protected:
	CNWPVideoEditorDoc() noexcept;
	DECLARE_DYNCREATE(CNWPVideoEditorDoc)

// Attributes
public:

// Operations
public:

// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// Implementation
public:
	virtual ~CNWPVideoEditorDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
};
