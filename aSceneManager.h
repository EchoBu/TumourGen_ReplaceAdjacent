#pragma once
#include <io.h>  
#include <fcntl.h>
#include <memory>
#include <string>
#include <vector>
#include "SiMath.h"

class iCathMesh;
class CxWireCore;
class ShaderLib;
class iCathCameraController;
class aSceneRender;
class aEvent;
class CxLines;
class Octree;
class WxSphere;
class CxTriangles;
class CxLine;
class CxPoints;

class aSceneManager
{
public:
	aSceneManager();
	~aSceneManager();


public:

	void Init(int x,int y);

	void Render();
	void Resize(int x,int y);
	
	void OnKeyEvent(int nChar);

	void OnEvent(const aEvent& e);

	void OpenMeshFile(const char* filePath);

	// Select A Point On The Screen (x,y) And Map It Back To World Space.
	// Create A Sphere Centered On This Point And Do Collision Detection With Vascular Mesh.
	// Based On Things We Have Done, Create A Tumor.
	void CreateTumor(int x,int y);

	std::shared_ptr<iCathCameraController>	GetCameraController(void);

	void SelectNarrowFace(int x,int y);
	void ClearNarrowPoint();

	bool GetTransparency() const;
	void SetTransparency(bool bTransparency);

	bool GetDrawWireframe() const;
	void SetDrawWireframe(bool bDrawWireframe);

	void fillSelectedHole();
	void removeDuplicates();
	void removeEdgeAndNeighborFaces();
	void reverseFace();

	void removeSelectFace();

	void selectMeshFace(int x,int y);

	void GenEmbolism();

	void SaveSTL();

	void SaveAsSVM();
	void SaveAsVM();
	void ReverseFacesAndSaveAsObj();
	void SaveAsObj();

	void SeperateTumor(int x,int y);

	void SpotTumors();

	void SpotNarrows();

	
	void CreateArapTumor(int x, int y,int flag);
	void PrecomputeForArap(int x, int y);
	void PrecomputeForArap_TumoeMode(int x, int y);
	void SetStartPoint(int x, int y);
	void ReturnToInit();
	void ReturnToLast();
	void ClearSelectedFace();		
	void SelectLittleTumorFace(int z, int y);	
	
	iv::vec3 start_point = iv::vec3(0,0,0);
	

private:
	
	std::string GetExtension(const char* fileName);
	std::string GetFileName(const char* filePath);

	std::vector<std::shared_ptr<iCathMesh> > m_pTumors;
	std::vector<std::shared_ptr<CxPoints> > m_pNarrows;

	int		mWidth;
	int		mHeight;

	iv::vec3  screenRayCollisionPoint(int x,int y);

	std::vector<iv::vec3>					vSelected;

	std::shared_ptr<ShaderLib>				mpShaderLib;
	std::shared_ptr<WxSphere>				m_pSphere;
	std::shared_ptr<CxTriangles>			m_pTriangles;
	std::shared_ptr<CxLines>				m_pLines;
	std::shared_ptr<CxLine>					m_pXAxis;
	std::shared_ptr<CxLine>					m_pYAxis;
	std::shared_ptr<CxLine>					m_pZAxis;

	std::shared_ptr<aSceneRender>			mpSceneRender;

	std::string								mCurrentMeshName;

	std::shared_ptr<iCathMesh>				mpVascularMesh;
	std::shared_ptr<iCathMesh>				m_pSeperatedTumorMesh;
	std::shared_ptr<iCathMesh>				m_pNarrowedMesh;

	int m_iActiveHoleEdgeIndex;

	double									m_dAlpha;
	double									m_dBeta;
	double									m_dRadius;
	iv::vec3								m_vCenter;

	std::shared_ptr<iCathCameraController>	mpCameraController;

	friend aSceneRender;
};