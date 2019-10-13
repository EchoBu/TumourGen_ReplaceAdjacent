
// AblationView.cpp : implementation of the CAblationView class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "Ablation.h"
#endif
#include "Parameters.h"
#include "AblationDoc.h"
#include "AblationView.h"
#include "Clock.h"
#include "xUtilMfc.h"
#include "aEvent.h"
#include "aCameraController.h"
#include "aSceneRender.h"
#include <thread>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAblationView

IMPLEMENT_DYNCREATE(CAblationView, CView)

BEGIN_MESSAGE_MAP(CAblationView, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CAblationView::OnFilePrintPreview)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_IMPORT_MODULE1, &CAblationView::OnImportModule1)
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOVE()
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_INSERT_GUIDECATHETER, &CAblationView::OnInsertGuidecatheter)
	ON_COMMAND(ID_INSERT_ABLATIONCATHETER, &CAblationView::OnInsertAblationcatheter)
	ON_COMMAND(ID_TUNE_PARAMETERS, &CAblationView::OnTuneParameters)
	ON_WM_KEYUP()
	ON_WM_MOUSEWHEEL()
	ON_COMMAND(ID_EXPORT_STL, &CAblationView::OnExportStl)
	ON_COMMAND(ID_RESTORE_HEALTH, &CAblationView::OnRestoreHealth)
	ON_COMMAND(ID_EXPORT_VM, &CAblationView::OnExportVm)
	ON_COMMAND(ID_SAVE_SAVE, &CAblationView::OnSaveSave)
	ON_COMMAND(ID_RENDER_WIREFRAME, &CAblationView::OnRenderWireframe)
	ON_COMMAND(ID_RENDER_TRANSPARENCY, &CAblationView::OnRenderTransparency)
	ON_COMMAND(ID_SAVE_SAVEASSVM, &CAblationView::OnSaveSaveassvm)
	ON_COMMAND(ID_RENDER_SPOTTUMORS, &CAblationView::OnRenderSpottumors)
	ON_COMMAND(ID_RENDER_SPOTNARROWS, &CAblationView::OnRenderSpotnarrows)
	ON_WM_MBUTTONDOWN()
	ON_COMMAND(ID_SAVE_AS, &CAblationView::OnSaveAs)
	ON_COMMAND(ID_MODIFY_FILLTHISHOLE, &CAblationView::OnModifyFillthishole)
	ON_COMMAND(ID_MODIFY_REMOVEDUPLICATES, &CAblationView::OnModifyRemoveduplicates)
	ON_COMMAND(ID_MODIFY_ROMOVEEDGEANDNEIGHBORFACES, &CAblationView::OnModifyRomoveedgeandneighborfaces)
	ON_COMMAND(ID_MODIFY_REVERSESELECTEDFACE, &CAblationView::OnModifyReverseselectedface)
	ON_COMMAND(ID_SAVE_REVERSETRIANGLEWINDINGSANDSAVEASVM, &CAblationView::OnSaveReversetrianglewindingsandsaveasvm)
	ON_COMMAND(ID_SAVE_AS32790, &CAblationView::OnSaveAs32790)
	ON_COMMAND(ID_MODE_NARROW32802, &CAblationView::OnChooseNarrowMode)
	ON_COMMAND(ID_MODE_TUMOR32801, &CAblationView::OnChooseSmallTumorMode)
	ON_COMMAND(ID_MODE_TUMOR32799, &CAblationView::OnChooseTumorMode)

END_MESSAGE_MAP()

// CAblationView construction/destruction

CAblationView::CAblationView() : mbSceneInited(false),
	mLeftCtlDown(false),
	mLeftButtonDown(false),
	m_bLeftShiftDown(false),	
	mDeformMode(Narrow_Mode),
	m_bFaceSelectMode(false)
{
	// TODO: add construction code here
}

CAblationView::~CAblationView()
{
}

BOOL CAblationView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CAblationView drawing

void CAblationView::OnDraw(CDC* /*pDC*/)
{
	CAblationDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: add draw code for native data here
}


// CAblationView printing


void CAblationView::OnFilePrintPreview()
{
#ifndef SHARED_HANDLERS
	AFXPrintPreview(this);
#endif
}

BOOL CAblationView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CAblationView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CAblationView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}



void CAblationView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{

#ifndef SHARED_HANDLERS
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_MENU1, point.x, point.y, this, TRUE);
#endif
	
}


// CAblationView diagnostics

#ifdef _DEBUG
void CAblationView::AssertValid() const
{
	CView::AssertValid();
}

void CAblationView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CAblationDoc* CAblationView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CAblationDoc)));
	return (CAblationDoc*)m_pDocument;
}
#endif //_DEBUG


// CAblationView message handlers


int CAblationView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	UtilMFC::showWin32Console();

	// TODO:  Add your specialized creation code here
	mGLContext.create30Context(GetSafeHwnd());
	mGLContext.makeCurrent(GetSafeHwnd());

	glClearColor(0.0f,0.0f,0.0f,1.0f);

	return 0;
}


void CAblationView::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: Add your message handler code here
	// Do not call CView::OnPaint() for painting messages
	PrintFPS();
	InitSceneManager();

	RenderScene();

	SwapBuffers(dc.m_ps.hdc);
}


void CAblationView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	mWidth = cx;
	mHeight = cy;

	mSmgr.Resize(cx,cy);
	// TODO: Add your message handler code here
}


BOOL CAblationView::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default

	return TRUE;
}

void CAblationView::RenderScene()
{

	mSmgr.Render();
}


void CAblationView::OnImportModule1()
{
}

void CAblationView::InitSceneManager()
{
	if( !mbSceneInited)
		mSmgr.Init(mWidth,mHeight);
	mbSceneInited = true;
}


void CAblationView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	switch(nChar)
	{
	case VK_CONTROL:
		{
			mLeftCtlDown = false;			
		}		
		break;
	case VK_SHIFT:
		{
			m_bLeftShiftDown = false;
			mSmgr.ClearNarrowPoint();
		}
		break;
	
	default:
		break;
	}

	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CAblationView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default

	switch(nChar)
	{
	case VK_CONTROL:
		mLeftCtlDown = true;
		break;
	case VK_SHIFT:
		m_bLeftShiftDown = true;
		break;
	case 'R':
		mSmgr.reverseFace();
		break;
	case 'D':
		mSmgr.removeSelectFace();
		break;
	/*case 'B':
		mSmgr.ReturnToLast();
		break;*/
	case 'I':
		mSmgr.ReturnToInit();
		break;
	case '1':
		mDeformMode = Narrow_Mode;
		printf("Narrow_Mode : On!\n");
		break;
	case '2':
		mDeformMode = Small_Tumor_Mode;
		printf("Small_Tumor_Mode : On!\n");
		break;
	case '3':
		mDeformMode = Tumor_Mode;
		printf("Tumor_Mode : On!\n");
		break;
	default:
		break;
	}

	mSmgr.OnKeyEvent(nChar);
	InvalidateRect(NULL,FALSE);
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CAblationView::OnRButtonUp(UINT nFlags, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);	
}

void CAblationView::OnChooseNarrowMode()
{
	mDeformMode = Narrow_Mode;	
	printf("Narrow_Mode : On!\n");
}
void CAblationView::OnChooseSmallTumorMode()
{
	mDeformMode = Small_Tumor_Mode;
	printf("Small_Tumor_Mode : On!\n");
}
void CAblationView::OnChooseTumorMode()
{
	mDeformMode = Tumor_Mode;
	printf("Tumor_Mode : On!\n");
}

void CAblationView::OnLButtonDown(UINT nFlags, CPoint point)
{
	mLeftButtonDown = true;
	// TODO: Add your message handler code here and/or call default
	if(!mbSceneInited )
		return;	
	
	else 
	{
		if (!mLeftCtlDown && !m_bLeftShiftDown)
			mSmgr.GetCameraController()->HandleMouse(point.x, point.y, Grab);
		
		else if (mLeftCtlDown) 
		{
			switch (mDeformMode)
			{
			case CAblationView::Narrow_Mode:
				mSmgr.SetStartPoint(point.x, point.y);
				mSmgr.PrecomputeForArap(point.x, point.y);//进行ARAP预处理
				break;
			case CAblationView::Small_Tumor_Mode:
								
				mSmgr.SetStartPoint(point.x, point.y);
				mSmgr.PrecomputeForArap(point.x, point.y);//进行ARAP预处理
				break;
			case CAblationView::Tumor_Mode:
				mSmgr.SetStartPoint(point.x, point.y);
				mSmgr.selectMeshFace(point.x, point.y);				
				mSmgr.PrecomputeForArap_TumoeMode(point.x, point.y);//进行ARAP预处理	
				break;
			default:
				break;
			}						
		}			
		else if(m_bLeftShiftDown)
		{	
			switch (mDeformMode)
			{
			case Narrow_Mode:
				mSmgr.ClearSelectedFace();
				mSmgr.SelectNarrowFace(point.x, point.y);
				break;
			case Small_Tumor_Mode:
				mSmgr.SelectLittleTumorFace(point.x, point.y);//
				break;
			case Tumor_Mode:
				mSmgr.CreateTumor(point.x, point.y);//构造血管瘤以及不规则突起凹陷			
				break;
			default:
				break;
			}			
					
		}
	}
	InvalidateRect(NULL,FALSE);
	CView::OnLButtonDown(nFlags, point);
}


void CAblationView::OnLButtonUp(UINT nFlags, CPoint point)
{

	// TODO: Add your message handler code here and/or call default
	mLeftButtonDown = false;
	
	mSmgr.GetCameraController()->HandleMouse(point.x, point.y, Release);

	InvalidateRect(NULL, FALSE);
	CView::OnLButtonUp(nFlags, point);
}


void CAblationView::OnMove(int x, int y)
{
	CView::OnMove(x, y);

	// TODO: Add your message handler code here
}


void CAblationView::OnMouseMove(UINT nFlags, CPoint point)
{
	if(!mbSceneInited)
		return;	

	if (mLeftCtlDown&&mLeftButtonDown)
	{		
		switch (mDeformMode)
		{
		case CAblationView::Narrow_Mode:
			mSmgr.CreateArapTumor(point.x, point.y, 0);//doDeform，0代表构造狭窄
			mSmgr.SetStartPoint(point.x, point.y);
			break;
		case CAblationView::Small_Tumor_Mode:
			mSmgr.CreateArapTumor(point.x, point.y, 1);//doDeform，1代表构造凸起
			mSmgr.SetStartPoint(point.x, point.y);
			break;
		case CAblationView::Tumor_Mode:
			mSmgr.CreateArapTumor(point.x, point.y, 1);
			mSmgr.SetStartPoint(point.x, point.y);
			break;
		default:
			break;
		}
	}
	
	else if (m_bLeftShiftDown&&mLeftButtonDown)
	{

		
	}
	else 
	{
		// TODO: Add your message handler code here and/or call default
		mSmgr.GetCameraController()->HandleMouse(point.x, point.y, Drag);
		InvalidateRect(NULL, FALSE);
		CView::OnMouseMove(nFlags, point);
	}	
}



void CAblationView::OnInsertGuidecatheter()
{

}


void CAblationView::OnInsertAblationcatheter()
{
		

}

void CAblationView::PrintFPS( void )
{
	static double tLast = iv::Clock::GetCurrentTimeMS();
	static int nFrames = 0;

	double tCurrent = iv::Clock::GetCurrentTimeMS();

	if( tCurrent - tLast > 1000 )
	{
		CString str;

		str.Format("FPS: %d\n",nFrames);
		CMFCStatusBar* pStatus = (CMFCStatusBar*)AfxGetMainWnd()->GetDescendantWindow(AFX_IDW_STATUS_BAR);
		if( pStatus )
			pStatus->SetPaneText(0,(LPCTSTR)str);
		nFrames = 0;
		tLast = tCurrent;
	}
	else
		++nFrames;
}


void CAblationView::OnTuneParameters()
{
	// TODO: Add your command handler code here
	Parameters dlgParameters;
	dlgParameters.DoModal();
}





BOOL CAblationView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: Add your message handler code here and/or call default

	float fStep = 0.05f;

	if(!mbSceneInited )
		return FALSE;
	if( zDelta < 0 )
		mSmgr.GetCameraController()->HandleMouse(0,0,ScrollZoomIn);
	else
		mSmgr.GetCameraController()->HandleMouse(0,0,ScrollZoomOut);

	InvalidateRect(NULL,FALSE);

	return CView::OnMouseWheel(nFlags, zDelta, pt);
}


void CAblationView::OnExportStl()
{
	// TODO: Add your command handler code here
}


void CAblationView::OnRestoreHealth()
{
	// TODO: Add your command handler code here
}


void CAblationView::OnExportVm()
{
	// TODO: Add your command handler code here
}


void CAblationView::OnSaveSave()
{
	// TODO: Add your command handler code here
	mSmgr.SaveSTL();
}


void CAblationView::OnRenderWireframe()
{
	// TODO: Add your command handler code here
	mSmgr.SetDrawWireframe(!mSmgr.GetDrawWireframe());
}


void CAblationView::OnRenderTransparency()
{
	// TODO: Add your command handler code here
	mSmgr.SetTransparency(!mSmgr.GetTransparency());
}


void CAblationView::OnSaveSaveassvm()
{
	// TODO: Add your command handler code here
	mSmgr.SaveAsSVM();
}


void CAblationView::OnRenderSpottumors()
{
	// TODO: Add your command handler code here
	mSmgr.SpotTumors();
}


void CAblationView::OnRenderSpotnarrows()
{
	// TODO: Add your command handler code here
	mSmgr.SpotNarrows();
}


void CAblationView::OnMButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	CView::OnMButtonDown(nFlags, point);
}


void CAblationView::OnSaveAs()
{
	// TODO: Add your command handler code here
	mSmgr.SaveAsVM();
}


void CAblationView::OnModifyFillthishole()
{
	// TODO: Add your command handler code here
	mSmgr.fillSelectedHole();
}


void CAblationView::OnModifyRemoveduplicates()
{
	// TODO: Add your command handler code here
	mSmgr.removeDuplicates();
}


void CAblationView::OnModifyRomoveedgeandneighborfaces()
{
	// TODO: Add your command handler code here
	mSmgr.removeEdgeAndNeighborFaces();
}


void CAblationView::OnModifyReverseselectedface()
{
	// TODO: Add your command handler code here
	mSmgr.reverseFace();
}


void CAblationView::OnSaveReversetrianglewindingsandsaveasvm()
{
	// TODO: Add your command handler code here
	mSmgr.ReverseFacesAndSaveAsObj();
}


void CAblationView::OnSaveAs32790()
{
	// TODO: Add your command handler code here
	mSmgr.SaveAsObj();
}
