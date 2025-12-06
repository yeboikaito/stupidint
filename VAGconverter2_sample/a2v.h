/* SIE CONFIDENTIAL
 * VAG Converter 2 version 2.5.0.3
 * Copyright(C) 2016 Sony Interactive Entertainment Inc.
 */

#pragma once

#include <afxwin.h>
#include "resource.h"

class Ca2vApp : public CWinApp
{
public:
	Ca2vApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern Ca2vApp theApp;
