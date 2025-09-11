#include "pch.h"
#include "framework.h"
#include "NWPVideoEditor.h"
#include "NWPVideoEditorDoc.h"
#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CNWPVideoEditorDoc, CDocument)

BEGIN_MESSAGE_MAP(CNWPVideoEditorDoc, CDocument)
END_MESSAGE_MAP()

CNWPVideoEditorDoc::CNWPVideoEditorDoc() noexcept {}
CNWPVideoEditorDoc::~CNWPVideoEditorDoc() {}

BOOL CNWPVideoEditorDoc::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;
    return TRUE;
}

void CNWPVideoEditorDoc::Serialize(CArchive& ar)
{
    if (ar.IsStoring())
    {
    }
    else
    {
    }
}

#ifdef _DEBUG
void CNWPVideoEditorDoc::AssertValid() const { CDocument::AssertValid(); }
void CNWPVideoEditorDoc::Dump(CDumpContext& dc) const { CDocument::Dump(dc); }
#endif
