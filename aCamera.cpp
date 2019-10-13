#include "stdafx.h"
#include <cstdint>

#include "aCamera.h"

using namespace iv;


Camera::Camera( vec3 p /*= iv::vec3(0.0f,0.0f,10.0f)*/, vec3 v /*= iv::vec3(0.0f)*/, vec3 u /*= iv::vec3(0.0f,0.0f,1.0f)*/, float n /*= 0.1f*/, float f /*= 1000.0f*/, float ratio /*= 1.75f*/, float fovy /*= 90.0f*/ )
{
	mPosition = p;
	mFocus = v;
	mUpOrientation = u;
	mNear = n;
	mFar = f;
	mRatio = ratio;
	mFovy = fovy;
	mClientWidth = 800;
	mClientWidth = 600;
}


Camera::~Camera()
{

}


void Camera::SetFrustum( float fovy,float ratio,float nearClip,float farClip )
{
	mFovy = fovy;
	mRatio = ratio;
	mNear = nearClip;
	mFar = farClip;
}

void Camera::SetPosition( const vec3& p )
{
	mPosition = p;
}

void Camera::SetUpOrientation( const vec3& u )
{
	mUpOrientation = u;
}

mat4 Camera::GetViewMatrix( void )
{
	return lookAt(mPosition,mFocus,mUpOrientation);
}

iv::mat4 Camera::GetProjectionMatrix( void )
{
	return perspective(mFovy,mRatio,mNear,mFar);
}

iv::vec3 Camera::GetFaceOrientation()
{
	return normalize(mFocus - mPosition);
}

float Camera::GetFocusLength()
{
	return length(mFocus - mPosition);
}

void Camera::SetNearClip( float n )
{
	mNear = n;
}

void Camera::SetFarClip( float f )
{
	mFar = f;
}

void Camera::SetRatio( float r )
{
	mRatio = r;
}

void Camera::SetFovy( float fovy )
{
	mFovy = fovy;
}

int Camera::GetViewportWidth() const
{
	return mClientWidth;
}

int Camera::GetViewportHeight() const
{
	return mClientHeight;
}

iv::vec3 Camera::GetPosition() const
{
	return mPosition;
}

iv::vec3 Camera::GetUpOrientation() const
{
	return mUpOrientation;
}

void Camera::SetViewportWidth( int w )
{
	mClientWidth = w;
}

void Camera::SetViewportHeight( int h )
{
	mClientHeight = h;
}

iv::vec3 Camera::GetFocus() const
{
	return mFocus;
}

void Camera::SetFoucs( const iv::vec3& f )
{
	mFocus = f;
}
