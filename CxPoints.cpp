#include "stdafx.h"
#include "CxPoints.h"

CxPoints::CxPoints() : m_hVao(0),
	m_hVbo(0),
	m_bDirty(true),
	m_iMaxPointCount(5000)
{
	PreAllocBuffer();
}

CxPoints::~CxPoints()
{
	if(m_hVao > 0)
		glDeleteVertexArrays(1,&m_hVao);
	if(m_hVbo > 0)
		glDeleteBuffers(1,&m_hVbo);
}

void CxPoints::PreAllocBuffer()
{
	glGenVertexArrays(1,&m_hVao);
	glBindVertexArray(m_hVao);

	glGenBuffers(1,&m_hVbo);	
	glBindBuffer(GL_ARRAY_BUFFER,m_hVbo);
	
	glBufferData(GL_ARRAY_BUFFER,3 * sizeof(float) * m_iMaxPointCount,0,GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);

	glBindVertexArray(0);
}

void CxPoints::AddPoint( const iv::vec3& p )
{
	m_vPoints.push_back(p);
	SetDirty();
}

void CxPoints::SetDirty()
{
	m_bDirty = true;
}

void CxPoints::Clear()
{
	m_vPoints.clear();
	SetDirty();
}

void CxPoints::UploadData()
{
	if(!m_bDirty)
		return;

	glBindVertexArray(m_hVao);
	
	glBindBuffer(GL_ARRAY_BUFFER,m_hVbo);
	glBufferSubData(GL_ARRAY_BUFFER,0,m_vPoints.size() * sizeof(iv::vec3),&m_vPoints[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);

	glBindVertexArray(0);
	
	m_bDirty = false;
}

void CxPoints::Render()
{
	UploadData();
	glBindVertexArray(m_hVao);
	glDrawArrays(GL_POINTS,0,m_vPoints.size());
	glBindVertexArray(0);
}
