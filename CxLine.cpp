#include "stdafx.h"
#include "CxLine.h"



CxLine::CxLine() : m_hVao(0),m_hVbo(0),p1(0.0f,0.0f,0.0f),p2(0.0f,1.0f,0.0f),m_fStep(0.01f)
{
	glGenVertexArrays(1,&m_hVao);
	glBindVertexArray(m_hVao);
	glGenBuffers(1,&m_hVbo);
	glBindBuffer(GL_ARRAY_BUFFER,m_hVbo);
	iv::vec3 pos[2] = {p1,p2};
	glBufferData(GL_ARRAY_BUFFER,2 * sizeof(iv::vec3),pos,GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);
	glBindVertexArray(0);
}

CxLine::~CxLine()
{
	if(m_hVao > 0)
		glDeleteVertexArrays(1,&m_hVao);
	if(m_hVbo > 0)
		glDeleteBuffers(1,&m_hVbo);
}

void CxLine::SetStartEndPoint( const iv::vec3& px1,const iv::vec3 px2 )
{
	this->p1 = px1;
	this->p2 = px2;
	SetDirty();
}

void CxLine::SetDirty()
{
	m_bDirty = true;
}

void CxLine::Render()
{
	UpdateData();
	glBindVertexArray(m_hVao);
	glDrawArrays(GL_LINES,0,2);
	glBindVertexArray(0);
}

void CxLine::UpdateData()
{
	if(m_bDirty)
	{
		glBindVertexArray(m_hVao);
		glBindBuffer(GL_ARRAY_BUFFER,m_hVbo);
		iv::vec3 pos[2] = {p1,p2};
		glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(iv::vec3)*2,pos);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);
		glBindVertexArray(0);
	}
}

void CxLine::SetStepLength( float fStep )
{
	m_fStep = fStep;
}

void CxLine::IncX()
{
	p2.x += m_fStep;
	SetDirty();
}

void CxLine::DecX()
{
	p2.x -= m_fStep;
	SetDirty();
}

void CxLine::IncY()
{
	p2.y += m_fStep;
	SetDirty();
}

void CxLine::DecY()
{
	p2.y -= m_fStep;
	SetDirty();
}

void CxLine::IncZ()
{
	p2.z += m_fStep;
	SetDirty();
}

void CxLine::DecZ()
{
	p2.z -= m_fStep;
	SetDirty();
}

void CxLine::OutputInfo()
{
	iv::vec3 vInsertOrient = iv::normalize(p2 - p1);
	printf("Insert Position: (%f,%f,%f).    Insert Orientation : (%f,%f,%f).\n",p1.x,p1.y,p1.z,vInsertOrient.x,vInsertOrient.y,vInsertOrient.z);
}
