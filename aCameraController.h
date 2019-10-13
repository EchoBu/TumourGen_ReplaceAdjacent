#pragma once

#include "SiMath.h"

class aEvent;

class Camera;

enum ArcballEvent {
	Grab = 0,
	Drag,
	Release,
	ScrollZoomIn,
	ScrollZoomOut
};

class iCathCameraController
{
public:
	iCathCameraController();
	~iCathCameraController();


public:

	void HandleMouse(int x,int y,ArcballEvent ae);
	void HandleKeyboard(const aEvent& e);

	void AttachCamera(Camera* pCam);


private:
	iv::vec3 GetArcballVector(int x,int y);

	void MoveCameraCloserToFocus();
	void MoveCameraFurtherFromFocus();
	void RotateCameraCW();
	void RotateCameraCCW();
	void MoveUpCameraAlongY();
	void MoveDownCameraAlongY();

private:

	Camera*		mpCam;

	bool		mbDisableMouseResponse;

	bool		mbLeftButtonDown;
	int			mLastX;
	int			mLastY;
};