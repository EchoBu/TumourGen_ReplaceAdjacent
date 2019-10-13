#pragma once

#include <glew.h>
#include "SiMath.h"

class CxLine
{
public:
	CxLine();
	~CxLine();

	void SetStartEndPoint(const iv::vec3& p1,const iv::vec3 p2);

	void Render();

	void IncX();
	void DecX();
	void IncY();
	void DecY();
	void IncZ();
	void DecZ();

	void OutputInfo();

	void SetStepLength(float fStep);

private:
	void SetDirty();
	void UpdateData();
private:

	float	m_fStep;

	GLuint m_hVao;
	GLuint m_hVbo;
	bool	m_bDirty;
	iv::vec3 p1;
	iv::vec3 p2;
};