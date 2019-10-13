#include "stdafx.h"
#include "Collider.h"
#include "CollisionSetting.h"
#include "aMesh.h"

using namespace Opcode;

CxCollisionTest::CxCollisionTest() : m_count(0),m_MeshModel(NULL),isInitAlready(FALSE),coefficient(2.4),numToCal(40)
{
	m_faces = NULL;
	m_verts = NULL;
	m_norms = NULL;
}

CxCollisionTest::~CxCollisionTest()
{
	if(m_faces) 
	{
		delete[] m_faces; m_faces = NULL;
	}
	if (m_verts)
	{
		delete[] m_verts; m_verts = NULL;
	}
	if (m_norms)
	{
		delete[] m_norms; m_norms = NULL;
	}
	
	if(m_MeshModel) delete m_MeshModel;

}

double CxCollisionTest::GetElasticForVessel(void)
{
	return coefficient;
}

void CxCollisionTest::IncreaseEV(void)
{
	coefficient += 1.0;
}

void CxCollisionTest::SetNumToCal(int num)
{
	numToCal = num;
}

void CxCollisionTest::DecreaseEV(void)
{
	coefficient -= 1.0;
}

void CxCollisionTest::SetElasticForVessel(double EV)
{
	coefficient = EV;
}

void CxCollisionTest::Init(iCathMesh* pMesh)
{
	if (!isInitAlready)
	{
		m_Sphere.mCenter = Point(0.0f,0.0f,0.0f);
		m_Sphere.mRadius = 0.5f;
		m_MeshModel = ModifyInterface(pMesh);
		isInitAlready = TRUE;
	}
}


void CxCollisionTest::InitEx( iCathMesh* pMesh )
{
	if (!isInitAlready)
	{
		m_Sphere.mCenter = Point(0.0f,0.0f,0.0f);
		m_Sphere.mRadius = 0.5f;
		m_MeshModel = ModifyInterfaceEx(pMesh);
		isInitAlready = TRUE;
	}
}


Opcode::Model* CxCollisionTest::ModifyInterfaceEx( iCathMesh* pMesh )
{
	std::map<int,int> mOld2New;
	std::map<int,int>::iterator itrX;
	
	udword m_nbfaces = pMesh->mFaceMap.size();
	udword m_nbverts = pMesh->mVertexMap.size();
	udword m_nbnorms = m_nbverts;
	
	m_faces = new udword[m_nbfaces*3];
	m_verts = new Point[m_nbverts];
	m_norms = new Point[m_nbnorms];

	// Face Vertex Index
	int i = 0;
	for(iCathMesh::FaceMap::iterator itr = pMesh->mFaceMap.begin(); itr!=pMesh->mFaceMap.end(); ++itr,++i)
	{
		const iCathMesh::Face& rFace = itr->second;
		for(int j = 0; j < 3; ++j)
		{
			int iV = rFace.mVerticesID[j];
			itrX = mOld2New.find(iV);
			if(itrX != mOld2New.end())
			{
				m_faces[3*i+j] = itrX->second;
			}
			else
			{
				int iNew = mOld2New.size();
				m_faces[3*i+j] = iNew;
				mOld2New[iV] = iNew;
			}
		}
	}

	//new ArrayVertex
	i = 0;
	for(iCathMesh::VertexMap::iterator itr = pMesh->mVertexMap.begin();
		itr!= pMesh->mVertexMap.end(); ++itr,++i)
	{
		const iv::vec3& rv = itr->second.mPositions;
		const iv::vec3& rn = itr->second.mNormals;
		m_verts[i] = Point(rv.x,rv.y,rv.z);
		m_norms[i] = Point(rn.x,rn.y,rn.z);
	}

	m_MeshInterface.SetNbTriangles(m_nbfaces);
	m_MeshInterface.SetNbVertices(m_nbverts);
	m_MeshInterface.SetPointers((const IndexedTriangle*)m_faces, m_verts);

	Opcode::OPCODECREATE m_Create;
	m_Create.mIMesh                  = &m_MeshInterface;
	m_Create.mSettings.mLimit  = 1;
	m_Create.mSettings.mRules = Opcode::SPLIT_SPLATTER_POINTS|Opcode::SPLIT_GEOM_CENTER;
	m_Create.mNoLeaf			   = true;
	m_Create.mQuantized	       = true;
	m_Create.mKeepOriginal	   = false;
	m_Create.mCanRemap		   = false;

	m_MeshModel = new Opcode::Model;
	if (!m_MeshModel->Build(m_Create))
	{
	}
	return m_MeshModel;
}



void CxCollisionTest::SetSphere(const Point &center,float radius)
{
	m_Sphere.mCenter = center;
	m_Sphere.mRadius = radius;
}

Opcode::Model* CxCollisionTest::ModifyInterface(iCathMesh* pMesh)
{
	udword m_nbfaces = pMesh->mFaceMap.size();
	udword m_nbverts = pMesh->mVertexMap.size();
	udword m_nbnorms = m_nbverts;
	m_faces = new udword[m_nbfaces*3];
	m_verts = new Point[m_nbverts];
	m_norms = new Point[m_nbnorms];

	// Face Vertex Index
	int i = 0;
	for(iCathMesh::FaceMap::iterator itr = pMesh->mFaceMap.begin(); itr!=pMesh->mFaceMap.end(); ++itr,++i)
	{
		m_faces[3*i] = itr->second.mVerticesID[0];
		m_faces[3*i+1] = itr->second.mVerticesID[1];
		m_faces[3*i+2] = itr->second.mVerticesID[2];
	}

	//new ArrayVertex
	i = 0;
	for(iCathMesh::VertexMap::iterator itr = pMesh->mVertexMap.begin();
		itr!= pMesh->mVertexMap.end(); ++itr,++i)
	{
		const iv::vec3& rv = itr->second.mPositions;
		const iv::vec3& rn = itr->second.mNormals;
		m_verts[i] = Point(rv.x,rv.y,rv.z);
		m_norms[i] = Point(rn.x,rn.y,rn.z);
	}

	m_MeshInterface.SetNbTriangles(m_nbfaces);
	m_MeshInterface.SetNbVertices(m_nbverts);
	m_MeshInterface.SetPointers((const IndexedTriangle*)m_faces, m_verts);

	Opcode::OPCODECREATE m_Create;
	m_Create.mIMesh                  = &m_MeshInterface;
	m_Create.mSettings.mLimit  = 1;
	m_Create.mSettings.mRules = Opcode::SPLIT_SPLATTER_POINTS|Opcode::SPLIT_GEOM_CENTER;
	m_Create.mNoLeaf			   = true;
	m_Create.mQuantized	       = true;
	m_Create.mKeepOriginal	   = false;
	m_Create.mCanRemap		   = false;

	m_MeshModel = new Opcode::Model;
	if (!m_MeshModel->Build(m_Create))
	{
	}
	return m_MeshModel;
}


void CxCollisionTest::PerformSphereMeshDetection( iv::vec3 center,float radius, std::vector<int>& result )
{
	bool m_isContacted = false;
	const Opcode::Model* m_Model = m_MeshModel;

	if (!m_Model)
		return;

	Opcode::SphereCollider m_SphereCollider;
	m_Settings.SetupCollider(m_SphereCollider);

	bool m_Status = false;
	Point pos(center.x,center.y,center.z);
	assert(!(pos.x != pos.x));
	m_Status = m_SphereCollider.Collide(m_SphereCache,Sphere(pos,(float)radius),*m_Model,0,0);
	if(!m_Status)
		return;
	if(m_SphereCollider.GetContactStatus())
	{
		result.clear();
		m_isContacted = true;
		udword m_nbTris = m_SphereCollider.GetNbTouchedPrimitives();
		const udword* m_Indices = m_SphereCollider.GetTouchedPrimitives();
		for( udword i = 0; i < m_nbTris ; ++i )
			result.push_back(m_Indices[i]);
	}
}

iv::dvec3 CxCollisionTest::GetContactForce(udword nbTris,const udword* Indices,const Sphere &sphere)
{
	Point mCenter(sphere.mCenter);
	const udword* faces = m_faces;
	const Point* normals = m_norms;
	const Point* verts = m_verts;
	Point force(0.0,0.0,0.0);
	Point vert0(0,0,0),vert1(0,0,0),vert2(0,0,0),norm0(0,0,0),norm1(0,0,0),norm2(0,0,0);

	for (udword i = 0 ; i < nbTris ; i++)
	{
		norm0.Zero();
		norm1.Zero();
		norm2.Zero();
		udword index = *Indices++;

		vert0.x = verts[faces[index*3]].x;
		vert0.y = verts[faces[index*3]].y;
		vert0.z = verts[faces[index*3]].z;

		norm0.x += normals[faces[index*3]].x;
		norm0.y += normals[faces[index*3]].y;
		norm0.z += normals[faces[index*3]].z;

		vert1.x = verts[faces[index*3+1]].x;
		vert1.y = verts[faces[index*3+1]].y;
		vert1.z = verts[faces[index*3+1]].z;

		norm1.x += normals[faces[index*3]].x;
		norm1.y += normals[faces[index*3]].y;
		norm1.z += normals[faces[index*3]].z;

		vert2.x = verts[faces[index*3+2]].x;
		vert2.y = verts[faces[index*3+2]].y;
		vert2.z = verts[faces[index*3+2]].z;

		norm2.x += normals[faces[index*3]].x;
		norm2.y += normals[faces[index*3]].y;
		norm2.z += normals[faces[index*3]].z;

		Point kDiff = vert0 - mCenter;

		Point TriEdge0 = vert1 - vert0;
		Point TriEdge1 = vert2 - vert0;

		//Point kDiff = vert0 - mCenter;
		float fA00 = TriEdge0.SquareMagnitude();
		float fA01 = TriEdge0 | TriEdge1;
		float fA11 = TriEdge1.SquareMagnitude();
		float fB0 = kDiff | TriEdge0;
		float fB1 = kDiff | TriEdge1;
		float fC = kDiff.SquareMagnitude();
		float fDet = fabsf(fA00*fA11 - fA01*fA01);
		float u = fA01*fB1-fA11*fB0;
		float v = fA01*fB0-fA00*fB1;
		float SqrDist;

		if(u + v <= fDet)
		{
			if(u < 0.0f)
			{
				if(v < 0.0f)  // region 4
				{
					if(fB0 < 0.0f)
					{
						// v = 0.0f;
						if(-fB0>=fA00) { /*u = 1.0f;*/ SqrDist = fA00+2.0f*fB0+fC; }
						else { u = -fB0/fA00; SqrDist = fB0*u+fC; }
					}
					else
					{
						// u = 0.0f;
						if(fB1>=0.0f) { /*v = 0.0f;*/ SqrDist = fC; }
						else if(-fB1>=fA11) { /*v = 1.0f;*/ SqrDist = fA11+2.0f*fB1+fC; }
						else { v = -fB1/fA11; SqrDist = fB1*v+fC; }
					}
				}
				else  // region 3
				{
					// u = 0.0f;
					if(fB1>=0.0f) { /*v = 0.0f;*/ SqrDist = fC; }
					else if(-fB1>=fA11) { /*v = 1.0f;*/ SqrDist = fA11+2.0f*fB1+fC; }
					else { v = -fB1/fA11; SqrDist = fB1*v+fC; }
				}
			}
			else if(v < 0.0f)  // region 5
			{
				// v = 0.0f;
				if(fB0>=0.0f) { /*u = 0.0f;*/ SqrDist = fC; }
				else if(-fB0>=fA00) { /*u = 1.0f;*/ SqrDist = fA00+2.0f*fB0+fC; }
				else { u = -fB0/fA00; SqrDist = fB0*u+fC; }
			}
			else  // region 0
			{
			// minimum at interior point
				if(fDet==0.0f)
				{
					// u = 0.0f;
					// v = 0.0f;
					SqrDist = MAX_FLOAT;
				}
				else
				{
					float fInvDet = 1.0f/fDet;
					u *= fInvDet;
					v *= fInvDet;
					SqrDist = u*(fA00*u+fA01*v+2.0f*fB0) + v*(fA01*u+fA11*v+2.0f*fB1)+fC;
				}
			}
		}
		else
		{
			float fTmp0, fTmp1, fNumer, fDenom;

			if(u < 0.0f)  // region 2
			{
				fTmp0 = fA01 + fB0;
				fTmp1 = fA11 + fB1;
				if(fTmp1 > fTmp0)
				{
					fNumer = fTmp1 - fTmp0;
					fDenom = fA00-2.0f*fA01+fA11;
					if(fNumer >= fDenom)
					{
						// u = 1.0f;
						// v = 0.0f;
						SqrDist = fA00+2.0f*fB0+fC;
					}
					else
					{
						u = fNumer/fDenom;
						v = 1.0f - u;
						SqrDist = u*(fA00*u+fA01*v+2.0f*fB0) + v*(fA01*u+fA11*v+2.0f*fB1)+fC;
					}
				}
				else
				{
				// u = 0.0f;
					if(fTmp1 <= 0.0f) { /*v = 1.0f;*/ SqrDist = fA11+2.0f*fB1+fC; }
					else if(fB1 >= 0.0f) { /*v = 0.0f;*/ SqrDist = fC; }
					else { v = -fB1/fA11; SqrDist = fB1*v+fC; }
				}
			}
			else if(v < 0.0f)  // region 6
			{
				fTmp0 = fA01 + fB1;
				fTmp1 = fA00 + fB0;
				if(fTmp1 > fTmp0)
				{
					fNumer = fTmp1 - fTmp0;
					fDenom = fA00-2.0f*fA01+fA11;
					if(fNumer >= fDenom)
					{
						// v = 1.0f;
						// u = 0.0f;
						SqrDist = fA11+2.0f*fB1+fC;
					}
					else
					{
						v = fNumer/fDenom;
						u = 1.0f - v;
						SqrDist = u*(fA00*u+fA01*v+2.0f*fB0) + v*(fA01*u+fA11*v+2.0f*fB1)+fC;
					}
				}
				else
				{
					// v = 0.0f;
					if(fTmp1 <= 0.0f) { /*u = 1.0f;*/ SqrDist = fA00+2.0f*fB0+fC; }
					else if(fB0 >= 0.0f) { /*u = 0.0f;*/ SqrDist = fC; }
					else { u = -fB0/fA00; SqrDist = fB0*u+fC; }
				}
			}
			else  // region 1
			{
				fNumer = fA11 + fB1 - fA01 - fB0;
				if(fNumer <= 0.0f)
				{
					// u = 0.0f;
					// v = 1.0f;
					SqrDist = fA11+2.0f*fB1+fC;
				}
				else
				{
					fDenom = fA00-2.0f*fA01+fA11;
					if(fNumer >= fDenom)
					{
						// u = 1.0f;
						// v = 0.0f;
						SqrDist = fA00+2.0f*fB0+fC;
					}
					else
					{
						u = fNumer/fDenom;
						v = 1.0f - u;
						SqrDist = u*(fA00*u+fA01*v+2.0f*fB0) + v*(fA01*u+fA11*v+2.0f*fB1)+fC;
					}
				}
			}
		}
		Point norms(0.0,0.0,0.0);
		float iswhere = (vert0 - mCenter) | ((vert1 - mCenter) ^ (vert2 - mCenter));
		norms.Cross(TriEdge0,TriEdge1);
		norms.Normalize();

		assert(!(SqrDist!=SqrDist));
		
		if (SqrDist < 0.0)	SqrDist = 0.0f; //有极少数情况出现了小于0，但是其绝对值远远<<<<0。

		if (iswhere < 0.0)   //包围盒中心跑到球外面去了
			force += -(sphere.mRadius+sqrt(SqrDist))*norms;
		else
			force += -(sphere.mRadius-sqrt(SqrDist))*norms;		

		/*if (iswhere < 0.0)   //包围盒中心跑到球外面去了
			force += -(sphere.mRadius - iswhere)*norms;
		else
			force += -(sphere.mRadius + iswhere)*norms;*/	


	}
	return iv::dvec3(force.x,force.y,force.z);
}


int CxCollisionTest::RayMeshCollision( const iv::vec3& b, const iv::vec3& o )
{
	const Opcode::Model* m_Model = m_MeshModel;

	if (!m_Model)
		return -1;


	Opcode::RayCollider rayCollider;


	rayCollider.SetFirstContact(true);
	rayCollider.SetCulling(false);
	rayCollider.SetHitCallback(OnRayHit);

	Point s(b.x,b.y,b.z);

	Point d(o.x,o.y,o.z);
	d.Normalize();

	UserData ud;
	ud.ray = Ray(s,d);

	rayCollider.SetUserData(&ud);
	static udword RayCache;

	bool result = false;

	result = rayCollider.Collide(Ray(s,d),*m_Model,0,&RayCache);

	if( !result )
		return -1;

	return ud.hitFaceID;
}

void CxCollisionTest::OnRayHit( const Opcode::CollisionFace& hit,void* user_data )
{
	UserData* ud = (UserData*)user_data;
	Point hitPos = ud->ray.mDir * hit.mDistance + ud->ray.mOrig;

	ud->hitFaceID = hit.mFaceID;

	RayHitPoint = hitPos;

	printf("Hit position:(%f,%f,%f)\n",hitPos.x,hitPos.y,hitPos.z);
	//printf("Collision Face Index: %d. Hit Distance: %f\n",hit.mFaceID,hit.mDistance);
}

void CxCollisionTest::GetCollisionTriangesWithSphere( const iv::vec3 &vCenter,float fRadius,std::vector<iv::vec3>& o_vTriangls )
{
	bool m_isContacted = false;
	const Opcode::Model* m_Model = m_MeshModel;

	if (!m_Model)
		return;

	Opcode::SphereCollider m_SphereCollider;
	m_Settings.SetupCollider(m_SphereCollider);

	bool m_Status = false;
	Point pos(vCenter.x,vCenter.y,vCenter.z);
	assert(!(pos.x != pos.x));
	m_Status = m_SphereCollider.Collide(m_SphereCache,Sphere(pos,(float)fRadius),*m_Model,0,0);
	if(!m_Status)
		return;
	if(m_SphereCollider.GetContactStatus())
	{
		o_vTriangls.clear();
		m_isContacted = true;
		udword m_nbTris = m_SphereCollider.GetNbTouchedPrimitives();
		const udword* m_Indices = m_SphereCollider.GetTouchedPrimitives();

		for( udword i = 0; i < m_nbTris ; ++i )
		{
			udword index = *(m_Indices+i);
			
			iv::vec3 v1;
			v1.x = m_verts[m_faces[index*3]].x;
			v1.y = m_verts[m_faces[index*3]].y;
			v1.z = m_verts[m_faces[index*3]].z;
			o_vTriangls.push_back(v1);

			iv::vec3 v2;
			v2.x = m_verts[m_faces[index*3+1]].x;
			v2.y = m_verts[m_faces[index*3+1]].y;
			v2.z = m_verts[m_faces[index*3+1]].z;
			o_vTriangls.push_back(v2);

			iv::vec3 v3;
			v3.x = m_verts[m_faces[index*3+2]].x;
			v3.y = m_verts[m_faces[index*3+2]].y;
			v3.z = m_verts[m_faces[index*3+2]].z;
			o_vTriangls.push_back(v3);
		}
	}
}

void CxCollisionTest::GetCollisionTriangleIDWithSphere( const iv::vec3 &vCenter,float fRadius,std::vector<int> &o_vIDs )
{
	bool m_isContacted = false;
	const Opcode::Model* m_Model = m_MeshModel;


	if (!m_Model)
		return;
	
	Opcode::SphereCollider m_SphereCollider;
	m_Settings.SetupCollider(m_SphereCollider);

	bool m_Status = false;
	Point pos(vCenter.x,vCenter.y,vCenter.z);
	assert(!(pos.x != pos.x));
	m_Status = m_SphereCollider.Collide(m_SphereCache,Sphere(pos,(float)fRadius),*m_Model,0,0);
	if(!m_Status)
		return;
	if(m_SphereCollider.GetContactStatus())
	{
		o_vIDs.clear();
		m_isContacted = true;
		udword m_nbTris = m_SphereCollider.GetNbTouchedPrimitives();
		const udword* m_Indices = m_SphereCollider.GetTouchedPrimitives();

		for( udword i = 0; i < m_nbTris ; ++i )
		{
			udword index = *(m_Indices+i);
			o_vIDs.push_back(index);
		}
	}
}

Point CxCollisionTest::RayHitPoint = Point(0.0f,0.0f,0.0f);


