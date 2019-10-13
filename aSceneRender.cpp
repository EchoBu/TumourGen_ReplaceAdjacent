#include "stdafx.h"
#include "aSceneManager.h"
#include "aSceneRender.h"
#include "aMesh.h"
#include "GLShader.h"
#include "ShaderLib.h"
#include "aCamera.h"
#include "glutils.h"
#include "Collider.h"
#include "sphere.h"
#include "CxLines.h"
#include "CxTriangles.h"
#include "CxPoints.h"
#include "CxLine.h"

aSceneRender::aSceneRender( aSceneManager* pMgr ) : mpMgr(pMgr),
	m_bWireframe(true),
	mpCamera(new Camera),
	m_bTransparency(false),
	mLightPosition(0.0f,0.0f,0.0f),
	mLightIntensity(0.45f),
	mClientRect(0,0,800,600)
{
}


aSceneRender::~aSceneRender()
{

}



void aSceneRender::Init( iv::ivec4 rect )
{
	mClientRect = rect;
	mpCamera->SetRatio(static_cast<float>(mClientRect.z)/mClientRect.w);
	mpCamera->SetFovy(70.0f);
	mpCamera->SetViewportWidth(rect.z);
	mpCamera->SetViewportHeight(rect.w);
}

void aSceneRender::Resize( int x,int y )
{
	mClientRect = iv::ivec4(0,0,x,y);
	mpCamera->SetRatio(static_cast<float>(mClientRect.z)/mClientRect.w);
}

void aSceneRender::Update( double dt )
{

}

void aSceneRender::Render()
{

	if(!mpMgr->mpVascularMesh )
		return;
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	iv::mat4 viewMatrix			= mpCamera->GetViewMatrix();
	iv::mat4 projectionMatrix	= mpCamera->GetProjectionMatrix();

	std::shared_ptr<ShaderLib> pShaderLib = mpMgr->mpShaderLib;

	//Get The Shader Program Ptr We Will Use To Perform FBO-Texture-Based Multisample.
	GLShader* pProg = pShaderLib->GetShaderProgram(E_TEXTURE_MULTISAMPLE_PASS_ONE);

	pProg->Use();

	pProg->SetUniform("light.Intensity",mLightIntensity);
	pProg->SetUniform("light.Position",iv::vec4(mLightPosition,1.0f));
	pProg->SetUniform("ModelMatrix",iv::mat4(1.0f));
	pProg->SetUniform("ViewMatrix",viewMatrix);
	pProg->SetUniform("ProjectionMatrix",projectionMatrix);

	glViewport(mClientRect.x,mClientRect.y,mClientRect.z,mClientRect.w);

	GLUtils::checkForOpenGLError(__FILE__,__LINE__);

	if(m_bTransparency)
	{
		glEnable(GL_BLEND);
		glDepthMask(GL_FALSE);
		glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_COLOR);
	}

	GLUtils::checkForOpenGLError(__FILE__,__LINE__);

	// Draw Vascular 
	pProg->SetUniform("ModelMatrix",mpMgr->mpVascularMesh->getModelMatrix());
	pProg->SetUniform("material.Ka",iv::vec3(0.680392f,0.068627f,0.013725f));
	pProg->SetUniform("material.Kd",iv::vec3(0.680392f,0.068627f,0.013725f));
	pProg->SetUniform("material.Ks",iv::vec3(1.0f, 1.0f, 1.0f));
	pProg->SetUniform("material.Shininess",84.0f);
	
	mpMgr->mpVascularMesh->Render();

	GLUtils::checkForOpenGLError(__FILE__,__LINE__);

	if(m_bTransparency)
	{
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}	

	if(m_bWireframe)
	{
		//********************************
		// Start Draw Wireframes.
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
		glEnable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(-0.5,-0.5);

		pProg->SetUniform("ModelMatrix",iv::mat4(1.0f));
		pProg->SetUniform("material.Ka",iv::vec3(1.0f,1.0f,1.0f));
		pProg->SetUniform("material.Kd",iv::vec3(1.0f,1.0f,1.0f));
		pProg->SetUniform("material.Ks",iv::vec3(1.0f, 1.0f, 1.0f));
		pProg->SetUniform("material.Shininess",84.0f);
		//mpMgr->mpVascularMesh->Render();

		for(int i = 0; i < mpMgr->m_pTumors.size(); ++i)
		{
			if(mpMgr->m_pTumors[i])
				mpMgr->m_pTumors[i]->Render();
		}


		if(mpMgr->m_pNarrowedMesh)
			mpMgr->m_pNarrowedMesh->Render();

		glDisable(GL_POLYGON_OFFSET_LINE);
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
		// End Draw Wireframes.
		//********************************
	}

	// Draw Collided Trianges.
	glLineWidth(2.0f);

	glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	glEnable(GL_POLYGON_OFFSET_LINE);
	glPolygonOffset(-1.0,-1.0);

	pProg->SetUniform("ModelMatrix",iv::mat4(1.0f));
	pProg->SetUniform("material.Ka",iv::vec3(.0f,.0f,1.0f));
	pProg->SetUniform("material.Kd",iv::vec3(.0f,.0f,1.0f));
	pProg->SetUniform("material.Ks",iv::vec3(.0f, .0f, 1.0f));
	pProg->SetUniform("material.Shininess",84.0f);

	glDisable(GL_DEPTH_TEST);

	mpMgr->m_pTriangles->Render();

	glEnable(GL_DEPTH_TEST);

	glDisable(GL_POLYGON_OFFSET_LINE);
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

	glLineWidth(1.0f);


	GLUtils::checkForOpenGLError(__FILE__,__LINE__);


	GLUtils::checkForOpenGLError(__FILE__,__LINE__);

	pProg->SetUniform("ModelMatrix",mpMgr->m_pSphere->getModelMatrix());
	pProg->SetUniform("material.Ka",iv::vec3(0.080392f,0.008627f,0.93725f));
	pProg->SetUniform("material.Kd",iv::vec3(0.080392f,0.0068627f,0.93725f));
	pProg->SetUniform("material.Ks",iv::vec3(1.0f, 1.0f, 1.0f));
	pProg->SetUniform("material.Shininess",84.0f);

	if(m_bTransparency)
	{
		glEnable(GL_BLEND);
		glDepthMask(GL_FALSE);
		glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_COLOR);
	}

//	mpMgr->m_pSphere->render();

	if(m_bTransparency)
	{
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}	


	pProg = pShaderLib->GetShaderProgram(E_LINE_SHADER);
	pProg->Use();
	pProg->SetUniform("MVP",projectionMatrix * viewMatrix * mpMgr->mpVascularMesh->getModelMatrix());

	glDisable(GL_DEPTH_TEST);
	glLineWidth(2.0f);

	pProg->SetUniform("LineColor",iv::vec3(1.0f,1.0f,1.0f));
	
	CxLines* pEdgeLines = mpMgr->mpVascularMesh->getEdgeLines();
	if(pEdgeLines)
		pEdgeLines->Render();

	pProg->SetUniform("LineColor",iv::vec3(0.0f,1.0f,0.0f));
	CxLines* pActiveLines = mpMgr->mpVascularMesh->getActiveEdgeLines();
	if(pActiveLines)
		pActiveLines->Render();

	pProg->SetUniform("LineColor",iv::vec3(0.0f,0.0f,1.0f));
	CxLines* pSelectedFace = mpMgr->mpVascularMesh->getSelectedFace();
	if(pSelectedFace)
		pSelectedFace->Render();

	
	glLineWidth(1.0f);
	glEnable(GL_DEPTH_TEST);

	

	pProg = pShaderLib->GetShaderProgram(E_POINTS);
	pProg->Use();
	pProg->SetUniform("MVP",projectionMatrix * viewMatrix);
	pProg->SetUniform("Color",iv::vec3(1.0f,1.0f,1.0f));
	glPointSize(2.0f);

	for(int i = 0; i < mpMgr->m_pNarrows.size(); ++i)
		mpMgr->m_pNarrows[i]->Render();

	glPointSize(1.0f);
	

}

std::shared_ptr<Camera> aSceneRender::GetCameraPtr()
{
	return mpCamera;
}

void aSceneRender::SetVascularTransparency( bool t )
{
	m_bTransparency = t;
}

bool aSceneRender::GetTransparency() const
{
	return m_bTransparency;
}

void aSceneRender::SetTransparency( bool bTransparency )
{
	m_bTransparency = bTransparency;
}

void aSceneRender::SetDrawWireframe( bool bDrawWireframe )
{
	m_bWireframe = bDrawWireframe;
}

bool aSceneRender::GetDrawWireframe( void ) const
{
	return m_bWireframe;
}

