/* SIE CONFIDENTIAL
 * VAG Converter 2 version 2.5.0.3
 * Copyright(C) 2016 Sony Interactive Entertainment Inc.
 */

#include "a2v.h"
#include "a2vDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

Ca2vApp theApp;

// Message Map

BEGIN_MESSAGE_MAP(Ca2vApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

// Construction

Ca2vApp::Ca2vApp()
{
}

// Initialization

BOOL Ca2vApp::InitInstance()
{
	CWinApp::InitInstance();

	Ca2vDlg dlg;

	m_pMainWnd = &dlg;

	dlg.DoModal();

	return FALSE;
}
