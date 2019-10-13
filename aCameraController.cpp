#include "stdafx.h"

#include <assert.h>

#include "aCameraController.h"
#include "aEvent.h"
#include "aCamera.h"

using namespace iv;

iCathCameraController::iCathCameraController() : mpCam(NULL),
	mbLeftButtonDown(false),
	mbDisableMouseResponse(false),
	mLastX(0),
	mLastY(0)
{

}

iCathCameraController::~iCathCameraController()
{

}

void iCathCameraController::HandleMouse( int x,int y,ArcballEvent ae )
{
	if(mbDisableMouseResponse)
		return;
	switch(ae)
	{
	case ScrollZoomIn:
		{
			MoveCameraCloserToFocus();
			break;
		}
	case ScrollZoomOut:
		{
			MoveCameraFurtherFromFocus();
			break;
		}
	case Grab:
		{
			mbLeftButtonDown = true;
			mLastX = x;
			mLastY = y;
		}
		break;
	case Release:
		{
			mbLeftButtonDown = false;
			mLastX = x;
			mLastY = y;
		}
		break;
	case Drag:
		{
			if(!mbLeftButtonDown)
				return;
			int CurrentX = x;
			int CurrentY = y;
			if( mLastX == CurrentX && mLastY == CurrentY )
				return;

			vec3 vA = GetArcballVector(mLastX,mLastY);
			vec3 vB = GetArcballVector(CurrentX,CurrentY);

			float rotRadian = acos(sil_min(1.0f,dot(vA,vB)));

			vec3 rotAxis = normalize(cross(vA,vB));

			mat4 camViewMatrix = mpCam->GetViewMatrix();
			mat3 cam2world = mat3(inverse(camViewMatrix));

			vec3 rotAxisInWorld = cam2world * rotAxis;

			vec3 camPosition = mpCam->GetPosition();
			vec3 camWorldUp = mpCam->GetUpOrientation();
			
			mat4 rotMat = rotate(-rotRadian,rotAxisInWorld);
			camWorldUp = mat3(rotMat) * camWorldUp;
			camPosition = vec3(rotMat* vec4(camPosition,1.0f));

			mpCam->SetPosition(camPosition);
			mpCam->SetUpOrientation(camWorldUp);

			mLastX = CurrentX;
			mLastY = CurrentY;
		}
		break;
	}
}

iv::vec3 iCathCameraController::GetArcballVector( int x,int y )
{
	assert(mpCam);

	double rectWidth = (double)mpCam->GetViewportWidth();
	double rectHeight = (double)mpCam->GetViewportHeight();

	iv::vec3 P( 2.0 *(double)x / rectWidth - 1.0,
		2.0*(double)y / rectHeight - 1.0,
		0.0);
	P.y = -P.y;
	float OP_squared = P.x*P.x+ P.y* P.y;
	if(OP_squared <= 1.0)
		P.z = sqrt(1.0f - OP_squared);
	else
		P = iv::normalize(P);
	return P;
}

void iCathCameraController::AttachCamera( Camera* pCam )
{
	assert(pCam != NULL);
	mpCam = pCam;
}

void iCathCameraController::HandleKeyboard( const aEvent& e )
{
}

void iCathCameraController::MoveCameraCloserToFocus()
{
	assert(mpCam);
	const float moveSpeed = 1.0f;
	vec3 camPos = mpCam->GetPosition();
	vec3 camOrientation = mpCam->GetFaceOrientation();
	mpCam->SetPosition(camPos + camOrientation * moveSpeed);
}

void iCathCameraController::MoveCameraFurtherFromFocus()
{
	assert(mpCam);
	const float moveSpeed = 1.0f;
	vec3 camPos = mpCam->GetPosition();
	vec3 camOrientation = mpCam->GetFaceOrientation();
	mpCam->SetPosition(camPos - camOrientation * moveSpeed);
}

void iCathCameraController::RotateCameraCW()
{
	assert(mpCam);
	const float rotateSpeed = ivHALFPI / 60.0f;
	mat4 rotMat = rotate(rotateSpeed,vec3(0.0f,1.0f,0.0f));
	vec3 camPos = mpCam->GetPosition();
	mpCam->SetPosition(mat3(rotMat) * camPos);
}

void iCathCameraController::RotateCameraCCW()
{
	assert(mpCam);
	const float rotateSpeed = ivHALFPI / 60.0f;
	mat4 rotMat = rotate(-rotateSpeed,vec3(0.0f,1.0f,0.0f));
	vec3 camPos = mpCam->GetPosition();
	mpCam->SetPosition(mat3(rotMat) * camPos);
}

void iCathCameraController::MoveUpCameraAlongY()
{
	assert(mpCam);
	const float moveSpeed = 0.5f;
	vec3 camPos = mpCam->GetPosition();
	vec3 camFocus =  mpCam->GetFocus();
	camPos.y -= moveSpeed;
	camFocus.y -= moveSpeed;
	mpCam->SetPosition(camPos);
	mpCam->SetFoucs(camFocus);
}

void iCathCameraController::MoveDownCameraAlongY()
{
	assert(mpCam);
	const float moveSpeed = 0.5f;
	vec3 camPos = mpCam->GetPosition();
	vec3 camFocus =  mpCam->GetFocus();
	camPos.y += moveSpeed;
	camFocus.y += moveSpeed;
	mpCam->SetPosition(camPos);
	mpCam->SetFoucs(camFocus);
}

