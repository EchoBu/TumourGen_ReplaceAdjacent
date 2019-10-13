#include "stdafx.h"
#include <assert.h>
#include <fstream>
#include <map>
#include <cstdint>
#include <set>
#include "aSceneManager.h"
#include "aSceneRender.h"
#include "aCameraController.h"
#include "aCamera.h"
#include "CxIO.h"
#include "aEvent.h"
#include "aMesh.h"
#include "ShaderLib.h"
#include "SiMath.h"
#include "Clock.h"
#include "glutils.h"
#include "sphere.h"
#include "CxLines.h"
#include "CxLine.h"
#include "CxTriangles.h"
#include "CxPoints.h"
#include <algorithm>


aSceneManager::aSceneManager() : mWidth(800),
	mHeight(600),
	mpShaderLib(NULL),
	mpSceneRender(NULL),
	m_pSphere(0),
	mpVascularMesh(NULL),
	mpCameraController(NULL),
	m_pNarrowedMesh(NULL),
	m_pLines(0),
	m_pTriangles(0),
	m_pSeperatedTumorMesh(0),
	m_dAlpha(0.0),
	m_dBeta(0.0),
	m_dRadius(2.0),
	m_vCenter(0.0f),
	m_iActiveHoleEdgeIndex(0)
{
	
}

aSceneManager::~aSceneManager()
{

}

void aSceneManager::Init( int x,int y )
{
	mWidth = x;
	mHeight = y;

	m_pTriangles = std::shared_ptr<CxTriangles>(new CxTriangles);

	mpShaderLib = std::shared_ptr<ShaderLib>(new ShaderLib);
	mpShaderLib->CompileAndLinkAllShaderPrograms();

	
	mpSceneRender = std::shared_ptr<aSceneRender>( new aSceneRender(this));
	mpSceneRender->Init(iv::ivec4(0,0,mWidth,mHeight));

	mpCameraController = std::shared_ptr<iCathCameraController>(new iCathCameraController);
	mpCameraController->AttachCamera(mpSceneRender->GetCameraPtr().get());

	m_pLines = std::shared_ptr<CxLines>(new CxLines);

	m_pSphere = std::shared_ptr<WxSphere>(new WxSphere(0.15f));

	iv::vec3 origin(0.0f);


// 	GenEmbolism();
// 	GenEmbolism();

	m_pXAxis = std::shared_ptr<CxLine>(new CxLine);
	m_pXAxis->SetStartEndPoint(origin,iv::vec3(5.0f,0.0f,0.0f));
	m_pYAxis = std::shared_ptr<CxLine>(new CxLine);
	m_pYAxis->SetStartEndPoint(origin,iv::vec3(0.0f,5.0f,0.0f));
	m_pZAxis = std::shared_ptr<CxLine>(new CxLine);
	m_pZAxis->SetStartEndPoint(origin,iv::vec3(0.0f,0.0f,5.0f));

// 	m_pLines->AddVertex(iv::vec3(0.0f,0.0f,0.0f));
// 	m_pLines->AddVertex(iv::vec3(0.0f,1.0f,0.0f));
// 	m_pLines->AddVertex(iv::vec3(2.0f,0.0f,0.0f));
// 	m_pLines->AddVertex(iv::vec3(0.0f,0.0f,4.0f));
	
}

void aSceneManager::Render()
{
	assert(mpSceneRender);
	mpSceneRender->Update(0.0f);
	mpSceneRender->Render();
}

void aSceneManager::Resize( int x,int y )
{
	mWidth = x;
	mHeight = y;
	if( mpSceneRender)
		mpSceneRender->Resize(x,y);
}

void aSceneManager::OnEvent( const aEvent& e )
{
	switch(e.mSrc)
	{
	case GuideTube:
		break;
	case AblationTube:
		break;
	case Other:
		break;
	default:
		break;
	}
}


std::shared_ptr<iCathCameraController> aSceneManager::GetCameraController( void )
{
	return mpCameraController;
}


void aSceneManager::OpenMeshFile( const char* filePath )
{
	std::string fileExt = GetExtension(filePath);
	mCurrentMeshName = GetFileName(filePath);

	if(fileExt == "nar")
	{
		CxImporter::loadNarFile(filePath);
		return;
	}

	if(mpVascularMesh)
		mpVascularMesh.reset();


	if (fileExt == std::string("stl") || fileExt == std::string("STL"))
		mpVascularMesh = std::shared_ptr<iCathMesh>(CxImporter::LoadSTLFile(filePath));
	else if (fileExt == std::string("obj") || fileExt == std::string("OBJ"))
		mpVascularMesh = std::shared_ptr<iCathMesh>(CxImporter::LoadObjFile(filePath));
	else if( fileExt == std::string("vm"))
		mpVascularMesh = std::shared_ptr<iCathMesh>(CxImporter::LoadVMBinFile(filePath));
	else if( fileExt == std::string("svm"))
		mpVascularMesh = std::shared_ptr<iCathMesh>(CxImporter::LoadSVMBinFile(filePath));
	else if(fileExt == std::string("nxvm"))
		mpVascularMesh = std::shared_ptr<iCathMesh>(CxImporter::LoadNxVMBinFile(filePath));
	else
		MessageBox(NULL,"Failed to open file. \nUnsupported format.","Failure",MB_OK);

	iv::mat4 modelMat = iv::translate(mpVascularMesh->getBoundingBox().vCenter * -1.0f);
	mpVascularMesh->setModelMatrix(modelMat);

	if(!mpVascularMesh)
		return;
	
	mpVascularMesh->GenAdjacency();
	mpVascularMesh->CalcPerFaceNormal();
	mpVascularMesh->CalcPerVertexNormal();
}

void aSceneManager::CreateTumor(int x, int y)
{
	
	iv::vec3 orig = screenRayCollisionPoint(x, y);
	//orig = iv::vec3(0.020857, -0.025091, 0.599364);
	
	float properRadius = mpVascularMesh->getVesselRadius(orig,0.02) * 0.5f;
	std::cout << "孔洞半径:" << properRadius << std::endl;
	if (properRadius == 0)
	{
		return;
	}
	m_pSphere->setPosition(orig);
	m_pSphere->SetRadius(properRadius);
	
	mpVascularMesh->OptimizedWayToGrowTumor(orig, m_pSphere->GetRadius());

	m_dRadius = mpVascularMesh->m_fRecentTumorRadius;
	m_vCenter = mpVascularMesh->m_vRecentTumorCenter;	
}


std::string aSceneManager::GetExtension( const char* fileName )
{
	std::string filePath(fileName);
	int dotPos = filePath.find_last_of('.');
	assert(dotPos > 0);
	std::string ext = filePath.substr(dotPos+1,filePath.length() - dotPos-1);
	std::cout<<ext<<std::endl;
	return ext;
}
std::string aSceneManager::GetFileName( const char* filePath )
{
	if(!filePath)
		return std::string("");
	std::string tmp(filePath);
	int p0 = tmp.find_last_of("\\");
	int p1 = tmp.find_last_of(".");
	std::string ret = tmp.substr(p0+1,p1-p0-1);
	return ret;
}

bool aSceneManager::GetTransparency() const
{
	return mpSceneRender->GetTransparency();
}

void aSceneManager::SetTransparency( bool bTransparency )
{
	mpSceneRender->SetTransparency(bTransparency);
}

void aSceneManager::OnKeyEvent( int nChar )
{
	switch(nChar)
	{
	case VK_F1:
		{
			++m_iActiveHoleEdgeIndex;
			if(mpVascularMesh)
				mpVascularMesh->setActiveEdgeIndex(m_iActiveHoleEdgeIndex);
		}
		break;
	case VK_F2:
		{
			--m_iActiveHoleEdgeIndex;
			if(mpVascularMesh)
				mpVascularMesh->setActiveEdgeIndex(m_iActiveHoleEdgeIndex);
		}
	case VK_SPACE:
		{
			for(int i = 0; i < 5; ++i)
				GenEmbolism();
		}
		break;
	case 'M':
		{
			if(mpVascularMesh)
				mpVascularMesh->RefineVertexPosition();
		}
		break;
	case 'N':
		{
			if(mpVascularMesh)
			{
				std::shared_ptr<iCathMesh> pMesh = mpVascularMesh->CreateSubMeshBySelectFace(0);
				if(pMesh)
				{
					std::string strName = mCurrentMeshName + "sb.vm";
					CxExporter::SaveAs_VM_Ex(pMesh.get(),strName.c_str());
				}
			}
		}
		break;
	case 'X':
		{
			if(m_pNarrowedMesh)
				m_pNarrowedMesh.reset();
			if(mpVascularMesh)
				//mpVascularMesh->NarrowMesh(9113,8366,&m_pNarrowedMesh);
				mpVascularMesh->NarrowMesh(7386,7383,&m_pNarrowedMesh);
		}
		break;
	default:
		break;
	}
}

void aSceneManager::SaveSTL()
{
	if(mpVascularMesh)
		mpVascularMesh->SaveAsSTL("TestTumor.stl");
}

bool aSceneManager::GetDrawWireframe() const
{
	if(mpSceneRender)
		return mpSceneRender->GetDrawWireframe();
	else
		return false;
}

void aSceneManager::SetDrawWireframe( bool bDrawWireframe )
{
	if(mpSceneRender)
		mpSceneRender->SetDrawWireframe(bDrawWireframe);
}

void aSceneManager::SeperateTumor( int x,int y )
{
	if(!mpSceneRender)
		return;

	mpSceneRender->SetVascularTransparency(false);
	mpSceneRender->Update(0.0);
	mpSceneRender->Render();

	GLfloat Dz = 1.0f;
	glReadPixels(x,mHeight-y,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,&Dz);

	printf("Depth: %f.\n",Dz);

	iv::vec3 src((float)x,(float)(mHeight-y),Dz);

	Camera* pCam = mpSceneRender->GetCameraPtr().get();
	assert(pCam);

	iv::vec4 vp(0.0f,0.0f,(float)mWidth,(float)mHeight);
	// orig 应该是点在世界坐标系下的点坐标

	// Get Cliked Position In World Space.
	iv::vec3 vCenter = iv::unproject(pCam->GetProjectionMatrix(),
		pCam->GetViewMatrix(),iv::mat4(1.0f),vp,src);
	float fRadius = 0.005f;

	m_pSphere->setPosition(vCenter);
	m_pSphere->SetRadius(fRadius);

	if(m_pSeperatedTumorMesh)
		m_pSeperatedTumorMesh.reset();

	m_pSeperatedTumorMesh = mpVascularMesh->SeperateTumor(vCenter,fRadius);

}

void aSceneManager::SaveAsSVM()
{
	if(mpVascularMesh)
	{
		std::string fn = mCurrentMeshName + std::string(".svm");
		CxExporter::SaveAsBinSVM(mpVascularMesh.get(),fn.c_str());
	}
}


void aSceneManager::SaveAsVM()
{
	if(mpVascularMesh)
	{
		std::string fn = mCurrentMeshName + std::string(".vm");
		CxExporter::SaveAs_VM_Ex(mpVascularMesh.get(),fn.c_str());
	}
}


void aSceneManager::SpotTumors()
{
	if(mpVascularMesh)
		mpVascularMesh->GetTumorSubMeshes(m_pTumors,true);
}

void aSceneManager::SelectNarrowFace( int x,int y )
{
	if (vSelected.size() > 2)
		return;

	if (!mpSceneRender)
		return;

	mpSceneRender->SetVascularTransparency(false);
	mpSceneRender->Update(0.0);
	mpSceneRender->Render();

	GLfloat Dz = 1.0f;
	glReadPixels(x, mHeight - y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &Dz);

	printf("Depth: %f.\n", Dz);

	iv::vec3 src((float)x, (float)(mHeight - y), Dz);

	Camera* pCam = mpSceneRender->GetCameraPtr().get();
	assert(pCam);

	iv::vec4 vp(0.0f, 0.0f, (float)mWidth, (float)mHeight);
	

	// Get Cliked Position In World Space.
	iv::vec3 vPick = iv::unproject(pCam->GetProjectionMatrix(),
		pCam->GetViewMatrix(), mpVascularMesh->getModelMatrix(), vp, src);

	printf("Picked Point(%f,%f,%f).\n", vPick.x, vPick.y, vPick.y);

	vSelected.push_back(vPick);

	if (vSelected.size() == 2)
	{
		if (m_pNarrowedMesh)
			m_pNarrowedMesh.reset();
		mpVascularMesh->NarrowMesh(vSelected[0], vSelected[1], &m_pNarrowedMesh);
	}
}


void aSceneManager::ClearNarrowPoint()
{
	vSelected.clear();
}


void aSceneManager::SpotNarrows()
{
	if(mpVascularMesh)
		mpVascularMesh->GetNarrowPoints(m_pNarrows);
}

void aSceneManager::GenEmbolism()
{
	static int iCalled = 1;
	static double dAlphaSpeed = iv::ivPI / 79;
	static double dBetaSpeed = iv::ivPI / 73;

	const double speed = iv::ivTWOPI / 100;
	const double speed2 = iv::ivTWOPI / 134;
	static double dsin = 0.0;
	static double dsin2 = 0.0;
	dAlphaSpeed = ( iv::ivPI / 79.0 ) * (sin(dsin) + 1.0) * 0.5;
	dBetaSpeed = (iv::ivPI / 73.0) * (sin(dsin2) + 1.0) * 0.5;

	dsin += speed;
	dsin2 += speed2;

	float z = m_dRadius * cos(m_dBeta) + m_vCenter.z;
	float x = m_dRadius * sin(m_dBeta) * cos(m_dAlpha) + m_vCenter.x;
	float y = m_dRadius * sin(m_dBeta) * sin(m_dAlpha) + m_vCenter.y;

	m_pLines->AddVertex(iv::vec3(x,y,z));

	m_dBeta += dBetaSpeed;
	if(m_dBeta >= iv::ivTWOPI)
		m_dBeta -= iv::ivTWOPI;
	m_dAlpha += dAlphaSpeed;
	if(m_dAlpha >= iv::ivTWOPI)
		m_dAlpha -= iv::ivTWOPI;
	
	++iCalled;
}

void aSceneManager::fillSelectedHole()
{
	if(mpVascularMesh)
		mpVascularMesh->fillActiveHole();
}

void aSceneManager::removeDuplicates()
{
	if(mpVascularMesh)
		mpVascularMesh->removeDuplicates();
}

void aSceneManager::removeEdgeAndNeighborFaces()
{
	if(mpVascularMesh)
		mpVascularMesh->removeThisEdgeAndNeighborFaces();
}

iv::vec3 aSceneManager::screenRayCollisionPoint(int x,int y)
{
	if(!mpSceneRender)
		return iv::vec3(0.0f);

	mpSceneRender->SetVascularTransparency(false);
	mpSceneRender->Update(0.0);
	mpSceneRender->Render();

	GLfloat Dz = 1.0f;
	glReadPixels(x,mHeight-y,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,&Dz);

	printf("Depth: %f.\n",Dz);

	iv::vec3 src((float)x,(float)(mHeight-y),Dz);

	Camera* pCam = mpSceneRender->GetCameraPtr().get();
	assert(pCam);

	iv::vec4 vp(0.0f,0.0f,(float)mWidth,(float)mHeight);
	
	// Get Cliked Position In World Space.
	iv::vec3 orig = iv::unproject(pCam->GetProjectionMatrix(),
		pCam->GetViewMatrix(),mpVascularMesh->getModelMatrix(),vp,src);

	printf("Clicked point position:(%f,%f,%f).\n",orig.x,orig.y,orig.z);

	return orig;
}

void aSceneManager::selectMeshFace(int x,int y)
{
	iv::vec3 p = screenRayCollisionPoint(x, y);
	std::cout << p.x << "," << p.y << "," << p.z << std::endl;

	if (mpVascularMesh)
		mpVascularMesh->SelectFaceForDeform_TumorMode(p);

}



void aSceneManager::SelectLittleTumorFace(int x,int y)
{
	iv::vec3 p = screenRayCollisionPoint(x, y);
	std::cout << p.x << "," << p.y << "," << p.z << std::endl;

	if (mpVascularMesh)
		mpVascularMesh->SelectLittleTumorFaces(p);
}

void aSceneManager::reverseFace()
{
	if(mpVascularMesh)
		mpVascularMesh->reverseSelectedFace();
}

void aSceneManager::removeSelectFace()
{
	if(mpVascularMesh)
		mpVascularMesh->removeSelectedFace();
}

void aSceneManager::ReverseFacesAndSaveAsObj()
{
	if(mpVascularMesh)
		mpVascularMesh->reverseFacesAndSaveAsObj();
}

void aSceneManager::SaveAsObj()
{
	if(mpVascularMesh)
		mpVascularMesh->saveAsObj("shit.obj");
}

void aSceneManager::PrecomputeForArap(int x, int y)
{
	iv::vec3 p = screenRayCollisionPoint(x, y);
	std::cout << p.x << "," << p.y << "," << p.z << std::endl;

	if (mpVascularMesh) {
		//start_point = p;
		mpVascularMesh->doArapmodelingPrecompute();
	}		
}
void aSceneManager::PrecomputeForArap_TumoeMode(int x, int y)
{
	iv::vec3 p = screenRayCollisionPoint(x, y);
	std::cout << p.x << "," << p.y << "," << p.z << std::endl;

	if (mpVascularMesh) {
		//start_point = p;
		mpVascularMesh->doArapmodelingPrecompute_TumorMode( );
	}
}

void aSceneManager::CreateArapTumor(int x, int y,int flag)
{
	iv::vec3 p = screenRayCollisionPoint(x, y);
	//std::cout << p.x << "," << p.y << "," << p.z << std::endl;

	if (mpVascularMesh)
	{
		// TODO 由于是计算鼠标初始点和终点在模型坐标系下的移动，
		// 所以鼠标移动需要在有网格的地方移动，否则offset会过大
		iv::vec3 offset = p - start_point;
		mpVascularMesh->doDeform(offset, flag);
	}
}
void aSceneManager::SetStartPoint(int x, int y)
{
	//iv::vec2 p = iv::vec2(x, y);
	iv::vec3 p = screenRayCollisionPoint(x, y);
	start_point = p;
}


void aSceneManager::ReturnToInit()
{
	if (mpVascularMesh)
		mpVascularMesh->getInitMesh();
	std::cout << "getInitMesh" << std::endl;
}

void aSceneManager::ReturnToLast()
{
	if (mpVascularMesh)
		mpVascularMesh->getBackupMesh();
	std::cout << "getBackupMesh" << std::endl;
}

void aSceneManager::ClearSelectedFace()
{

	if (mpVascularMesh)
		mpVascularMesh->clearSelectedFace();
	

}




