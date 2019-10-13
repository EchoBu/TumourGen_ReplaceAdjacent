// Parameters.cpp : implementation file
//

#include "stdafx.h"
#include "Ablation.h"
#include "Parameters.h"
#include "afxdialogex.h"
#include "AblationView.h"
#include "aSceneManager.h"
#include "Collider.h"


// Parameters dialog

IMPLEMENT_DYNAMIC(Parameters, CDialog)

Parameters::Parameters(CWnd* pParent /*=NULL*/)
	: CDialog(Parameters::IDD, pParent)
	, mFeedback(0)
	, mBoundingRadius(0)
	, mNodeElasticity(0)
	, mVesselElasticity(0)
{

}

Parameters::~Parameters()
{
}

void Parameters::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, FeedBack_EDIT4, mFeedback);
	DDX_Text(pDX, BoundingRadisu_EDIT, mBoundingRadius);
	DDX_Text(pDX, NodeElasticity_EDIT, mNodeElasticity);
	DDX_Text(pDX, VesselElastic_EDIT3, mVesselElasticity);
}


BEGIN_MESSAGE_MAP(Parameters, CDialog)
	ON_WM_CREATE()
	ON_BN_CLICKED(IDC_BUTTON_APPLY, &Parameters::OnBnClickedButtonApply)
END_MESSAGE_MAP()


// Parameters message handlers


int Parameters::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  Add your specialized creation code here
	// TODO: Add your specialized code here and/or call the base class
	CWnd* pMainWnd = AfxGetMainWnd();

	CView* pActiveView = static_cast<CFrameWnd*>(pMainWnd)->GetActiveFrame()->GetActiveView();

	CAblationView * pView = dynamic_cast<CAblationView*>(pActiveView);

	return 0;
}


void Parameters::OnBnClickedButtonApply()
{
	// TODO: Add your control notification handler code here

	UpdateData(TRUE);

	UpdateData(FALSE);

}
