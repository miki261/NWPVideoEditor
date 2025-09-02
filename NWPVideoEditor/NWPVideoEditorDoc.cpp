
// NWPVideoEditorDoc.cpp : implementation of the CNWPVideoEditorDoc class
//

#include "pch.h"
#include "framework.h"
#ifndef SHARED_HANDLERS
#include "NWPVideoEditor.h"
#endif

#include "NWPVideoEditorDoc.h"

#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CNWPVideoEditorDoc

IMPLEMENT_DYNCREATE(CNWPVideoEditorDoc, CDocument)

BEGIN_MESSAGE_MAP(CNWPVideoEditorDoc, CDocument)
END_MESSAGE_MAP()


// CNWPVideoEditorDoc construction/destruction

CNWPVideoEditorDoc::CNWPVideoEditorDoc() noexcept
{
}

CNWPVideoEditorDoc::~CNWPVideoEditorDoc()
{
}

BOOL CNWPVideoEditorDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	return TRUE;
}




// CNWPVideoEditorDoc serialization

void CNWPVideoEditorDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
	}
	else
	{
	}
}

#ifdef SHARED_HANDLERS

// Support for thumbnails
void CNWPVideoEditorDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void CNWPVideoEditorDoc::InitializeSearchContent()
{
	CString strSearchContent;
	SetSearchContent(strSearchContent);
}

void CNWPVideoEditorDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = nullptr;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != nullptr)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CNWPVideoEditorDoc diagnostics

#ifdef _DEBUG
void CNWPVideoEditorDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CNWPVideoEditorDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

