#pragma once


// Parameters dialog

class Parameters : public CDialog
{
	DECLARE_DYNAMIC(Parameters)

public:
	Parameters(CWnd* pParent = NULL);   // standard constructor
	virtual ~Parameters();

// Dialog Data
	enum { IDD = IDD_PARAMETERS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	double mFeedback;
	double mBoundingRadius;
	double mNodeElasticity;
	double mVesselElasticity;
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnBnClickedButtonApply();
};
