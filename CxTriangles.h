#pragma once

#include <glew.h>
#include <vector>

#include "SiMath.h"

class CxTriangles
{
public:
	CxTriangles();
	~CxTriangles();


	void AddTriangle(const iv::vec3& v1,const iv::vec3& v2,const iv::vec3& v3);

	void Render();

	void Clear();

private:
	void SetDirty();

	void AllocGpuBuffers();

	void UpdateGpuData();

private:
	GLuint m_hVao;
	GLuint m_hVbo[2];

	bool	m_bDirty;

	std::vector<iv::vec3> m_vTriangles;
};