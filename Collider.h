#ifndef __XCOLLISIONTEST_H_INCLUDED__
#define  __XCOLLISIONTEST_H_INCLUDED__

#include "CollisionSetting.h"
#include "SiMath.h"
#include <vector>


class iCathMesh;
class CxWireCore;

class CxCollisionTest
{
public:
	struct UserData
	{
		Ray ray;
		int hitFaceID;

		UserData() : hitFaceID(-1) {}
	};

public:
	CxCollisionTest();
	~CxCollisionTest();//三法则，有显式析构函数的类，也同时需要复制构造函数和复制操作符



private:
	int m_count;
	double coefficient;
	int numToCal;
	Opcode::Model* m_MeshModel;
	Opcode::MeshInterface m_MeshInterface;	
	Point* m_verts;
	Point* m_norms;
	udword* m_faces;
	bool isInitAlready;

public:
	static Point RayHitPoint;


public:

	void GetCollisionTriangesWithSphere(const iv::vec3 &vCenter,float fRadius,std::vector<iv::vec3>& o_vTriangls);
	void GetCollisionTriangleIDWithSphere(const iv::vec3 &vCenter,float fRadius,std::vector<int> &o_vIDs);

	static void OnRayHit(const Opcode::CollisionFace& hit,void* user_data);

	unsigned int PerformRayMeshDetection(CxWireCore* gw);
	int RayMeshCollision(const iv::vec3& b, const iv::vec3& d);

	void PerformSphereMeshDetection(iv::vec3 center,float radius,std::vector<int>& result);

	void IncreaseEV(void);
	void DecreaseEV(void);

	void SetElasticForVessel(double EV);
	double GetElasticForVessel(void);	
	
	
	void SetNumToCal(int num);

	// This Version Is For Map-Version iCathMath.
	void InitEx(iCathMesh* pMesh);
	Opcode::Model* ModifyInterfaceEx(iCathMesh* pMesh);

	void Init(iCathMesh* pMesh);
	Sphere m_Sphere;
	Opcode::SphereCache m_SphereCache;
	OpcodeSettings m_Settings;
	void SetSphere(const Point &center,float radius);
	Opcode::Model* ModifyInterface(iCathMesh* pMesh);
	bool PerformDetection(CxWireCore* gw);
	iv::dvec3 GetContactForce(udword nbTris,const udword* Indices,const Sphere &sphere);
};






#endif