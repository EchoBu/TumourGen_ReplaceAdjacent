#include "stdafx.h"
#include "CxLines.h"


CxLines::CxLines( int iMaxLines /*= 200*/ ) : m_hVao(0),
	m_hVbo(0),
	m_bDirty(false),
	m_iMaxLines(iMaxLines)
{
	PreAllocBuffers();
}

CxLines::~CxLines()
{
	if(m_hVao > 0)
		glDeleteVertexArrays(1,&m_hVao);
	if(m_hVbo > 0)
		glDeleteBuffers(1,&m_hVbo);
}

void CxLines::PreAllocBuffers()
{
	glGenVertexArrays(1,&m_hVao);
	glBindVertexArray(m_hVao);
	glGenBuffers(1,&m_hVbo);
	glBindBuffer(GL_ARRAY_BUFFER,m_hVbo);
	glBufferData(GL_ARRAY_BUFFER,m_iMaxLines * 2 * sizeof(iv::vec3),0,GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);
	glBindVertexArray(0);
}

void CxLines::UpdateGpuData()
{
	if(!m_bDirty)	return;

	if(m_vVertices.size() > 0)
	{
		glBindVertexArray(m_hVao);
		glBindBuffer(GL_ARRAY_BUFFER,m_hVbo);
		glBufferSubData(GL_ARRAY_BUFFER,0,m_vVertices.size() * sizeof(iv::vec3),&m_vVertices[0]);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);
		glBindVertexArray(0);
	}
	m_bDirty = false;
}

void CxLines::SetDirty()
{
	m_bDirty = true;
}

void CxLines::AddLines( const iv::vec3 &v1,const iv::vec3& v2 )
{
	m_vVertices.push_back(v1);
	m_vVertices.push_back(v2);
	SetDirty();
}

void CxLines::Render()
{
	UpdateGpuData();
	glBindVertexArray(m_hVao);
	glDrawArrays(GL_LINES,0,m_vVertices.size());
	//glDrawArrays(GL_LINE_STRIP,0,m_vVertices.size());
	glBindVertexArray(0);
}

void CxLines::Clear()
{
	m_vVertices.clear();
	SetDirty();
}

void CxLines::AddVertex( const iv::vec3 &v )
{
	m_vVertices.push_back(v);
	SetDirty();
}
