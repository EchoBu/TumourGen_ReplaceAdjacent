#pragma once


enum EventSrc {
	GuideTube,
	AblationTube,
	Other
};

enum EventDetail {

	GuideTube_Forwards,
	GuideTube_Backwards,
	GuideTube_RotateCW,
	GuideTube_RotateCCW,
	
	AblationTube_Forwards,
	AblationTube_Backwards,
	AblationTube_RotateCW,
	AblationTube_RotateCCW,
	AblationTube_BendTip,
	AblationTube_UnbendTip,

	Other_Undefined
};

class aEvent
{
public:
	EventSrc			mSrc;
	EventDetail			mDetail;

	aEvent() : mSrc(Other),
		mDetail(Other_Undefined)
	{

	}
};