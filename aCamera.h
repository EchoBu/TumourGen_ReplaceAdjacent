#pragma once

#include "SiMath.h"

class Camera
{
public:
	Camera(iv::vec3 p = iv::vec3(0.0f,0.0f,10.0f),
		iv::vec3 v = iv::vec3(0.0f),
		iv::vec3 u = iv::vec3(0.0f,1.0f,0.0f),
		float n = 0.1f,
		float f = 1000.0f,
		float ratio = 1.75f,
		float fovy = 90.0f);
	~Camera();

	void SetFrustum(float fovy,float ratio,float nearClip,float farClip);

	void SetNearClip(float n);

	void SetFarClip(float f);

	void SetPosition(const iv::vec3& p);

	void SetFovy(float fovy);

	void SetRatio(float r);

	void SetUpOrientation(const iv::vec3& u);

	void SetViewportWidth(int w);

	void SetViewportHeight(int h);

	iv::vec3 GetFocus() const;

	void SetFoucs( const iv::vec3& f);

	iv::vec3 GetPosition() const;

	iv::vec3 GetUpOrientation() const;

	iv::vec3 GetFaceOrientation();

	float GetFocusLength();

	int GetViewportWidth() const;

	int GetViewportHeight() const;
	iv::mat4 GetViewMatrix(void);
	iv::mat4 GetProjectionMatrix(void);

private:
	float	mNear;
	float	mFar;
	float	mRatio;
	float	mFovy;
	int		mClientWidth;
	int		mClientHeight;

	iv::vec3 mFocus;
	iv::vec3 mPosition;
	iv::vec3 mUpOrientation;


};