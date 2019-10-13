#pragma once

#include <glew.h>
#include <vector>
#include "SiMath.h"

class CxPoints
{
public:
	CxPoints();
	~CxPoints();

	void AddPoint(const iv::vec3& p);
	void Render();


	void Clear();

private:
	void PreAllocBuffer();
	void UploadData();
	void SetDirty();
private:
	GLuint m_hVao;
	GLuint m_hVbo;
	GLuint m_bDirty;
	GLuint m_iMaxPointCount;

	std::vector<iv::vec3> m_vPoints;
};