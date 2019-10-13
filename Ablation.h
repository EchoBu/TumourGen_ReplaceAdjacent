
// Ablation.h : main header file for the Ablation application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


// CAblationApp:
// See Ablation.cpp for the implementation of this class
//

class CAblationApp : public CWinAppEx
{
public:
	CAblationApp();


// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation
	UINT  m_nAppLook;
	BOOL  m_bHiColorIcons;

	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();

	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnIdle(LONG lCount);

	void InitConsoleWindow()
	{
		if (!AllocConsole() || !freopen("CONOUT$", "w", stdout))
			AfxMessageBox(_T("InitConsoleWindow Failed!")); //分配控制台在重定向输出流至控制台
	}
	
};

extern CAblationApp theApp;
