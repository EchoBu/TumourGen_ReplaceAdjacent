
// AblationView.h : interface of the CAblationView class
//

#pragma once
#include "glcontext.h"
#include "AblationDoc.h"
#include "aSceneManager.h"

class CAblationView : public CView
{
protected: // create from serialization only
	CAblationView();
	DECLARE_DYNCREATE(CAblationView)

// Attributes
public:
	CAblationDoc* GetDocument() const;

	OpenGLContext	mGLContext;

	aSceneManager	mSmgr;

	bool			mbSceneInited;

	bool			mLeftCtlDown;
	bool			m_bLeftShiftDown;
	

	bool			m_bFaceSelectMode;	
	bool			mLeftButtonDown;//add
	enum			DeformMode { Narrow_Mode = 1,Small_Tumor_Mode = 2, Tumor_Mode = 3 } mDeformMode;
	
	
	int		mWidth;
	int		mHeight;


// Operations
public:
	void InitSceneManager();
	void RenderScene();
	void PrintFPS(void);

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CAblationView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnImportModule1();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	
	afx_msg void OnMove(int x, int y);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnInsertGuidecatheter();
	afx_msg void OnInsertAblationcatheter();
	afx_msg void OnTuneParameters();
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnExportStl();
	afx_msg void OnRestoreHealth();
	afx_msg void OnExportVm();
	afx_msg void OnSaveSave();
	afx_msg void OnRenderWireframe();
	afx_msg void OnRenderTransparency();
	afx_msg void OnSaveSaveassvm();
	afx_msg void OnRenderSpottumors();
	afx_msg void OnRenderSpotnarrows();
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSaveAs();
	afx_msg void OnModifyFillthishole();
	afx_msg void OnModifyRemoveduplicates();
	afx_msg void OnModifyRomoveedgeandneighborfaces();
	afx_msg void OnModifyReverseselectedface();
	afx_msg void OnSaveReversetrianglewindingsandsaveasvm();
	afx_msg void OnSaveAs32790();
	afx_msg void OnChooseNarrowMode();
	afx_msg void OnChooseSmallTumorMode();
	afx_msg void OnChooseTumorMode();
	
};

#ifndef _DEBUG  // debug version in AblationView.cpp
inline CAblationDoc* CAblationView::GetDocument() const
   { return reinterpret_cast<CAblationDoc*>(m_pDocument); }
#endif

