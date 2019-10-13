#include "stdafx.h"
#include "CxTriangles.h"

CxTriangles::CxTriangles()
{
	glGenVertexArrays(1,&m_hVao);
	glGenBuffers(2,m_hVbo);

	AllocGpuBuffers();
}

CxTriangles::~CxTriangles()
{
	if(m_hVao > 0)
		glDeleteVertexArrays(1,&m_hVao);
	for(int i = 0; i < 2; ++i)
	{
		if(m_hVbo[i] > 0)
			glDeleteBuffers(1,&m_hVbo[i]);
	}
}

void CxTriangles::AllocGpuBuffers()
{
	const int iMaxTriangles = 2000;
	glBindVertexArray(m_hVao);
	glBindBuffer(GL_ARRAY_BUFFER,m_hVbo[0]);
	glBufferData(GL_ARRAY_BUFFER,iMaxTriangles * 3 * sizeof(iv::vec3),0,GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);


	glBindBuffer(GL_ARRAY_BUFFER,m_hVbo[1]);
	glBufferData(GL_ARRAY_BUFFER,iMaxTriangles * 3 * sizeof(iv::vec3),0,GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,0,0);

	glBindVertexArray(0);
}

void CxTriangles::AddTriangle( const iv::vec3& v1,const iv::vec3& v2,const iv::vec3& v3 )
{
	m_vTriangles.push_back(v1);
	m_vTriangles.push_back(v2);
	m_vTriangles.push_back(v3);
	m_bDirty = true;
}

void CxTriangles::UpdateGpuData()
{
	if(!m_bDirty)
		return;
	
	
	int iTriangles = m_vTriangles.size() / 3;

	printf("iTriangles : %d.\n",iTriangles);
	
	if(iTriangles == 0)
	{
		m_bDirty = false;
		return;
	}
	
	std::vector<iv::vec3> normals(iTriangles * 3);

	for(int i = 0; i < iTriangles; ++i)
	{
		const iv::vec3& v1 = m_vTriangles[3*i];
		const iv::vec3& v2 = m_vTriangles[3*i+1];
		const iv::vec3& v3 = m_vTriangles[3*i+2];

		normals[3*i] = iv::normalize(iv::cross(v3-v2,v1-v2));
		normals[3*i+1] = normals[3*i];
		normals[3*i+2] = normals[3*i];
	}

	glBindVertexArray(m_hVao);
	glBindBuffer(GL_ARRAY_BUFFER,m_hVbo[0]);
	glBufferSubData(GL_ARRAY_BUFFER,0,iTriangles * 3 * sizeof(iv::vec3),&m_vTriangles[0]);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);


	glBindBuffer(GL_ARRAY_BUFFER,m_hVbo[1]);
	glBufferSubData(GL_ARRAY_BUFFER,0,iTriangles * 3 * sizeof(iv::vec3),&normals[0]);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,0,0);

	glBindVertexArray(0);

	m_bDirty = false;
}

void CxTriangles::Clear()
{
	m_vTriangles.clear();
	m_bDirty = true;
}

void CxTriangles::Render()
{
	UpdateGpuData();
	glBindVertexArray(m_hVao);
	int iCount = m_vTriangles.size();
	glDrawArrays(GL_TRIANGLES,0,iCount);
	glBindVertexArray(0);
}
