#pragma once

#include <memory>

#include "SiMath.h"

class Camera;
class aSceneManager;

class aSceneRender
{
public:
	aSceneRender(aSceneManager* pMgr);
	~aSceneRender();

	void Render();
	void Update(double dt);
	void Init(iv::ivec4 rect);
	void Resize(int x,int y);

	void SetVascularTransparency(bool t);

	void SetDrawWireframe(bool bDrawWireframe);
	bool GetDrawWireframe(void) const;

	bool GetTransparency() const;
	void SetTransparency(bool bTransparency);

	std::shared_ptr<Camera>		GetCameraPtr();
private:

	bool						m_bWireframe;
	bool						m_bTransparency;
	aSceneManager*				mpMgr;
	std::shared_ptr<Camera>		mpCamera;

	iv::vec3					mLightPosition;
	iv::vec3					mLightIntensity;
	iv::ivec4					mClientRect;
};