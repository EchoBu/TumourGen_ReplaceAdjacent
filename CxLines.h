#pragma once

#include <glew.h>
#include <vector>
#include "SiMath.h"

class CxLines
{
public:
	CxLines(int iMaxLines = 20000);
	~CxLines();


public:
	void Clear();
	void Render();
	void AddLines(const iv::vec3 &v1,const iv::vec3& v2);
	void AddVertex(const iv::vec3 &v);

private:
	void PreAllocBuffers();
	void UpdateGpuData();
	void SetDirty();
private:
	GLuint	m_hVao;
	GLuint	m_hVbo;
	int		m_iMaxLines;
	bool	m_bDirty;

	std::vector<iv::vec3> m_vVertices;
};