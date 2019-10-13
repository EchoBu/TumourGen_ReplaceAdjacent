
#include "stdafx.h"
#include "aMesh.h"
#include "Clock.h"
#include "glutils.h"
#include <set>
#include <map>
#include <algorithm>
#include <fstream>
#include "CxPoints.h"
#include "Collider.h"
#include "CxLines.h"
#include <igl/upsample.h>
#include <windows.h>
#include <thread>

#include <cmath>
#include "GeodeticCalculator_AStar.h"
#include <stdlib.h>
#include <time.h> 
#include <Eigen\Dense>
#include <igl/decimate.h>

#include "SimplifyObject.h"
#include "MyPoint.h"



#define _USE_MATH_DEFINES
using namespace Eigen;

iCathMesh::iCathMesh() : m_bEdgeAdjacency(false),
	m_bFaceAdjacency(false),
	m_bVertexAdjacency(false)
{
	m_iErrorIndex = -1;
	m_bAdjacentyComplete = true;
	mbStarted = false;
	mStartTime = 0.0;
	mDuration = 3000.0;
	mhVao = 0;
	memset(mhVbo,0,sizeof(GLuint)*3);
	mbGpuLoaded = false;
	mbGpuAllocated = false;
	mbDirty = true;
	m_iActiveEdgeIndex = 0;
	m_ModelMatrix = iv::mat4(1.0f);
	m_iSelectedFaceIndex = -1;

	mFaceMapBackup = std::vector<FaceMap>();
	mVertexMapBackup = std::vector<VertexMap>();
	sum_b = 0.0f;
	mVesselRadius = 0.0f;
	mLengthBetweenTwoVPick = 0.0f;
	
}

iCathMesh::~iCathMesh()
{
	glDeleteVertexArrays(1,&mhVao);
	glDeleteBuffers(3,mhVbo);
}

#if USE_LEGACY
void iCathMesh::GenAdjacency( void )
{
	printf("Creating Mesh Adjacency Info...................");
	size_t faceCnt = mFaceMap.size();
	assert(faceCnt != 0);

	size_t vertexCnt = mVertexMap.size();
	assert(vertexCnt != 0);

	// Clear Stuff.
	for(FaceMap::iterator itr = mFaceMap.begin(); itr!=mFaceMap.end(); ++itr)
		itr->second.mNeighborFaceID.clear();
	for(VertexMap::iterator itr = mVertexMap.begin(); itr!=mVertexMap.end(); ++itr)
	{
		itr->second.mNeighborVertexID.clear();
		itr->second.mNeighborFaceID.clear();
	}

	// Find Every Vertex's Neighbor Face
	for(FaceMap::iterator itr = mFaceMap.begin(); itr!=mFaceMap.end(); ++itr)
	{
		Face& face = itr->second;
		for(int j = 0; j < 3; ++j)
			mVertexMap[face.mVerticesID[j]].mNeighborFaceID.push_back(itr->first);
	}

	// Create vertex neighbor for vertex;
	// !! Note that NeighborVertexID has duplication. There's a lot of room to optimize!	
	for(VertexMap::iterator itr = mVertexMap.begin(); itr!=mVertexMap.end(); ++itr)
	{
		Vertex& rVertex = itr->second;
		int faceNeighborCnt = rVertex.mNeighborFaceID.size();
		for(int j = 0; j < faceNeighborCnt; ++j)
		{
			const Face& rFace = mFaceMap[rVertex.mNeighborFaceID[j]];
			for(int k = 0; k < 3; ++k)
			{
				const Vertex& rV= mVertexMap[rFace.mVerticesID[k]];
				rVertex.mNeighborVertexID.push_back(rV.mID);
			}
		}
	}



	// Create face neighbor for face;
	// ! A Lot of room to optimize!

	for(FaceMap::iterator itr = mFaceMap.begin(); itr!=mFaceMap.end(); ++itr)
	{
		Face& face = itr->second;
		for(int j = 0; j < 3; ++j)
		{
			const Vertex& rVertexThis = mVertexMap[face.mVerticesID[j]];
			const Vertex& rVertexNext = mVertexMap[face.mVerticesID[(j+1)%3]];
			int faceNeighborCnt = rVertexThis.mNeighborFaceID.size();
			for(int k = 0; k < faceNeighborCnt; ++k)
			{
				const Face& faceNeighbor = mFaceMap[rVertexThis.mNeighborFaceID[k]];
				if(faceNeighbor.mID == face.mID)
					continue;
				if( j < face.mNeighborFaceID.size())
					continue;
				if( faceNeighbor.ContainVertexID(rVertexThis.mID) &&
					faceNeighbor.ContainVertexID(rVertexNext.mID))
					face.mNeighborFaceID.push_back(faceNeighbor.mID);
			}
		}
	}
	
	mEdges.clear();

	for(FaceMap::iterator itr = mFaceMap.begin(); itr!=mFaceMap.end(); ++itr)
	{
		const Face& rFace = itr->second;
		for(int j = 0; j < 3; ++j)
		{
			int iV1 = rFace.mVerticesID[j];
			int iV2 = rFace.mVerticesID[(j+1)%3];
			mEdges[Edge(iV1,iV2)].push_back(rFace.mID);
		}
	}

	printf("Done.\n");

	CheckAdjacentyComplete();
}

#else
void iCathMesh::GenAdjacency( void )
{
	printf("Creating Mesh Adjacency Info...................");

	GenVertexAdjacency();
	GenEdgeAdjacency();
	GenFaceAdjacency();

	printf("Done.\n");
	
	_createEdgeLines();

	CheckAdjacentyComplete();
	
}

#endif

void iCathMesh::Render()
{
	AllocateGpuBuffer();
	LoadUpToGpu();

	glBindVertexArray(mhVao);
	glDrawElements(GL_TRIANGLES,3*mFaceMap.size(),GL_UNSIGNED_INT,0);

	GLUtils::checkForOpenGLError(__FILE__,__LINE__);
}

void iCathMesh::LoadUpToGpu( void )
{
	if(mbGpuLoaded && !mbDirty)
		return;


	size_t faceCnt = mFaceMap.size();
	size_t vertexCnt = mVertexMap.size();

	unsigned int*	indices		= new unsigned int[faceCnt * 3];
	iv::vec3*		vertices	= new iv::vec3[vertexCnt];
	iv::vec3*		normals		= new iv::vec3[vertexCnt];

	int i = 0;
	for(FaceMap::iterator itr = mFaceMap.begin(); itr != mFaceMap.end(); ++itr,++i)
	{
		for(int j = 0; j < 3; ++j)
			indices[3*i+j] = itr->second.mVerticesID[j];
	}

	i = 0;
	for(VertexMap::iterator itr = mVertexMap.begin(); itr != mVertexMap.end(); ++itr,++i)
	{
		vertices[i] = itr->second.mPositions;
		normals[i] = itr->second.mNormals;
	}

	GLUtils::checkForOpenGLError(__FILE__,__LINE__);
	glBindVertexArray(mhVao);

	glBindBuffer(GL_ARRAY_BUFFER,mhVbo[0]);
	glBufferSubData(GL_ARRAY_BUFFER,0,vertexCnt * 3 * sizeof(float),vertices);
	GLUtils::checkForOpenGLError(__FILE__,__LINE__);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);

	glBindBuffer(GL_ARRAY_BUFFER,mhVbo[1]);
	glBufferSubData(GL_ARRAY_BUFFER,0,vertexCnt * 3 * sizeof(float),normals);
		GLUtils::checkForOpenGLError(__FILE__,__LINE__);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,0,0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mhVbo[2]);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,0,sizeof(unsigned int) * 3 * faceCnt,indices);
		GLUtils::checkForOpenGLError(__FILE__,__LINE__);
	delete[] indices;
	delete[] vertices;
	delete[] normals;

	glBindVertexArray(0);

	mbGpuLoaded = true;
	mbDirty = false;
}

void iCathMesh::AllocateGpuBuffer()
{
	if(mbGpuAllocated)
		return;


	size_t faceCnt = mFaceMap.size() * 2;
	size_t vertexCnt = mVertexMap.size() * 2;

	glGenVertexArrays(1,&mhVao);
	glBindVertexArray(mhVao);

	glGenBuffers(3,mhVbo);
	glBindBuffer(GL_ARRAY_BUFFER,mhVbo[0]);
	glBufferData(GL_ARRAY_BUFFER,vertexCnt * 3 * sizeof(float),0,GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);

	glBindBuffer(GL_ARRAY_BUFFER,mhVbo[1]);
	glBufferData(GL_ARRAY_BUFFER,vertexCnt * 3 * sizeof(float),0,GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,0,0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mhVbo[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(unsigned int) * 3 * faceCnt,0,GL_DYNAMIC_DRAW);

	glBindVertexArray(0);

	mbGpuAllocated = true;
}

void iCathMesh::SubDivisionTrianglesEx(std::vector<int>& vTriangles )
{
	int iTriangle = vTriangles.size();
	if(iTriangle == 0) return;

	std::map<Edge,int> vNew;
	std::map<Edge,int>::iterator itr;
	
	std::map<int,AuxFace> mAuxFace;
	std::map<int,AuxFace>::iterator AdjFaceItr;

	std::sort(vTriangles.begin(),vTriangles.end());

	std::vector<std::vector<int> > vEdgeAdj;
	CaculateAdjacentByEdge(vEdgeAdj,vTriangles);

	for(int i = 0; i < iTriangle; ++i)
	{
		int iIndex = vTriangles[i];

		Vertex v[3];

		for(int j = 0; j < 3; ++j)
		{
			Face &rFace = mFaceMap[iIndex];

			int iV1 = rFace.mVerticesID[j];
			int iV2 = rFace.mVerticesID[(j+1)%3];

			itr = vNew.find(Edge(iV1,iV2));

			// Already Created. Fetch It.
			if(itr!=vNew.end())	
				v[j] = mVertexMap[itr->second];
			else	// Create It
			{
				VertexMap::reverse_iterator itr = mVertexMap.rbegin();
				v[j].mPositions = ( mVertexMap[iV1].mPositions + mVertexMap[iV2].mPositions ) * 0.5f;
				v[j].mID = itr->first + 1;
				v[j].mNormals = normalize(mVertexMap[iV1].mNormals + mVertexMap[iV2].mNormals);
				mVertexMap[v[j].mID] = v[j];
				vNew[Edge(iV1,iV2)] = v[j].mID;
			}

			// Find neighbor face sharing this edge.
			int iNeighbor = vEdgeAdj[i][j];

			// If This Neighbor Is OutSide The Sphere, 
			// We Should Mark It And Deal With It Later.
			if(std::find(vTriangles.begin(),vTriangles.end(),iNeighbor) == vTriangles.end())
			{
				AdjFaceItr = mAuxFace.find(iNeighbor);

				// Already Marked. Shared By Two Triangles.
				// Two Edge Are Splited.
				if(AdjFaceItr != mAuxFace.end())
					AdjFaceItr->second.vNewVertices.push_back(MidVertex(Edge(iV1,iV2),v[j].mID));
				else	// Newly Marked.
				{
					mAuxFace[iNeighbor] = AuxFace(iNeighbor);
					mAuxFace[iNeighbor].vNewVertices.push_back(MidVertex(Edge(iV1,iV2),v[j].mID));
				}
			}
		}

		Face &rFace = mFaceMap[iIndex];

		// Now We Have All 3 Split Vertex. Next, We Split Current Face Into 4 Faces.
		// All We Have To Do Is Change The Index Of Vertex Composing The Face.

		int iOV1 = rFace.mVerticesID[0];
		int iOV2 = rFace.mVerticesID[1];
		int iOV3 = rFace.mVerticesID[2];

		// #1: Modify The Origin One.
		rFace.mVerticesID[1] = v[0].mID;
		rFace.mVerticesID[2] = v[2].mID;

		// #1: Add New Ones.
		FaceMap::reverse_iterator itr = mFaceMap.rbegin();

		Face newFace1;
		newFace1.mID = itr->first + 1;
		newFace1.mVerticesID.push_back(v[0].mID);
		newFace1.mVerticesID.push_back(iOV2);
		newFace1.mVerticesID.push_back(v[1].mID);
		mFaceMap[newFace1.mID] = newFace1;

		Face newFace2;
		newFace2.mID = newFace1.mID + 1;
		newFace2.mVerticesID.push_back(v[0].mID);
		newFace2.mVerticesID.push_back(v[1].mID);
		newFace2.mVerticesID.push_back(v[2].mID);
		mFaceMap[newFace2.mID] = newFace2;

		Face newFace3;
		newFace3.mID = newFace2.mID + 1;
		newFace3.mVerticesID.push_back(v[2].mID);
		newFace3.mVerticesID.push_back(v[1].mID);
		newFace3.mVerticesID.push_back(iOV3);
		mFaceMap[newFace3.mID] = newFace3;
	}


	// Now We Need To Handle mAuxFace.
	// For Every Face In mAuxFace, It Should Have At Least One MidVertex.
	for(AdjFaceItr = mAuxFace.begin(); AdjFaceItr!=mAuxFace.end(); AdjFaceItr++)
	{
		Face& rFace = mFaceMap[AdjFaceItr->first];
		SplitFaceByVertices(rFace.mID,AdjFaceItr->second.vNewVertices);
	}

	InValidAdjacencyInfo();
	
	GenAdjacency();

	mbDirty = true;
	
}




void iCathMesh::SplitFaceByVertices( int iFace2Split,const std::vector<MidVertex>& SplitVertices )
{
	Face& rFace = mFaceMap[iFace2Split];

	if(SplitVertices.size() == 1)
	{
		const MidVertex& rV = SplitVertices[0];
		int iOld = -1 ,iOldVertex = -1;
		for(int j = 0; j < 3; j++)
			if(rFace.mVerticesID[j] != rV.edge.iV1 && rFace.mVerticesID[j] != rV.edge.iV2)
				iOld = j;
		iOldVertex = rFace.mVerticesID[iOld];

		iv::vec3 vRefNormal = mVertexMap[iOldVertex].mNormals;

		rFace.mVerticesID[0] = iOldVertex;
		rFace.mVerticesID[1] = rV.edge.iV1;
		rFace.mVerticesID[2] = rV.iVertexID;

		CorrectWinding(rFace.mID,vRefNormal);

		FaceMap::reverse_iterator itr = mFaceMap.rbegin();

		Face newFace;
		newFace.mID = itr->first + 1;
		newFace.mVerticesID.push_back(iOldVertex);
		newFace.mVerticesID.push_back(rV.edge.iV2);
		newFace.mVerticesID.push_back(rV.iVertexID);
		mFaceMap[newFace.mID] = newFace;

		CorrectWinding(newFace.mID,vRefNormal);
	}

	if(SplitVertices.size() == 2)
	{
		printf("SplitFaceByVertex: Stage 2:!\n");

		int v[6];
		int iCursor = 0;
		int iEmpty = 0;
		for(int i = 0; i < 3; ++i)
		{
			int iV1 = rFace.mVerticesID[i];
			int iV2 = rFace.mVerticesID[(i+1)%3];
			v[iCursor++] = iV1;
			Edge tmp(iV1,iV2);
			if(tmp == SplitVertices[0].edge)
				v[iCursor++] = SplitVertices[0].iVertexID;
			else if(tmp == SplitVertices[1].edge)
				v[iCursor++] = SplitVertices[1].iVertexID;
			else
			{
				v[iCursor++] = -1;
				iEmpty = iCursor - 1;
			}
		}

		iv::vec3 vRefNormal = mVertexMap[rFace.mVerticesID[0]].mNormals;

		rFace.mVerticesID[0] = v[(iEmpty+1)%6];
		rFace.mVerticesID[1] = v[(iEmpty+2)%6];
		rFace.mVerticesID[2] = v[iEmpty-1];
		CorrectWinding(rFace.mID,vRefNormal);

		FaceMap::reverse_iterator itr = mFaceMap.rbegin();

		Face newFace1;
		newFace1.mID = itr->first + 1;
		newFace1.mVerticesID.push_back(v[(iEmpty+2)%6]);
		newFace1.mVerticesID.push_back(v[(iEmpty+3)%6]);
		newFace1.mVerticesID.push_back(v[(iEmpty+4)%6]);
		mFaceMap[newFace1.mID] = newFace1;
		CorrectWinding(newFace1.mID,vRefNormal);

		Face newFace2;
		newFace2.mID = newFace1.mID + 1;
		newFace2.mVerticesID.push_back(v[(iEmpty+2)%6]);
		newFace2.mVerticesID.push_back(v[(iEmpty+4)%6]);
		newFace2.mVerticesID.push_back(v[(iEmpty+5)%6]);
		mFaceMap[newFace2.mID] = newFace2;
		CorrectWinding(newFace2.mID,vRefNormal);
	}

	if(SplitVertices.size() == 3)
	{
		printf("SplitFaceByVertex: Stage 3:!\n");

		int v[6];
		int iCursor = 0;
		for(int i = 0; i < 3; ++i)
		{
			int iV1 = rFace.mVerticesID[i];
			int iV2 = rFace.mVerticesID[(i+1)%3];
			v[iCursor++] = iV1;
			Edge tmp(iV1,iV2);
			if(tmp == SplitVertices[0].edge)
				v[iCursor++] = SplitVertices[0].iVertexID;
			else if(tmp == SplitVertices[1].edge)
				v[iCursor++] = SplitVertices[1].iVertexID;
			else if(tmp == SplitVertices[2].edge)
				v[iCursor++] = SplitVertices[2].iVertexID;
			else
				printf("Error In Func. SplitFaceByVertex.\n");
		}

		iv::vec3 vRefNormal = mVertexMap[rFace.mVerticesID[0]].mNormals;

		rFace.mVerticesID[0] = v[0];
		rFace.mVerticesID[1] = v[1];
		rFace.mVerticesID[2] = v[5];
		CorrectWinding(rFace.mID,vRefNormal);

		FaceMap::reverse_iterator itr = mFaceMap.rbegin();

		Face newFace1;
		newFace1.mID = itr->first + 1;
		newFace1.mVerticesID.push_back(v[1]);
		newFace1.mVerticesID.push_back(v[2]);
		newFace1.mVerticesID.push_back(v[3]);
		mFaceMap[newFace1.mID] = newFace1;
		CorrectWinding(newFace1.mID,vRefNormal);

		Face newFace2;
		newFace2.mID = newFace1.mID + 1;
		newFace2.mVerticesID.push_back(v[1]);
		newFace2.mVerticesID.push_back(v[3]);
		newFace2.mVerticesID.push_back(v[5]);
		mFaceMap[newFace2.mID] = newFace2;
		CorrectWinding(newFace2.mID,vRefNormal);

		Face newFace3;
		newFace3.mID = newFace2.mID + 1;
		newFace3.mVerticesID.push_back(v[3]);
		newFace3.mVerticesID.push_back(v[4]);
		newFace3.mVerticesID.push_back(v[5]);
		mFaceMap[newFace3.mID] = newFace3;
		CorrectWinding(newFace2.mID,vRefNormal);

		
	}
}


int iCathMesh::GetNeighborFaceBySharedEdge( int iFaceID,int iV1,int iV2 )
{
	const Face& refFace = mFaceMap[iFaceID];
	for(int i = 0; i < refFace.mNeighborFaceID.size(); ++i)
	{
		const Face& f = mFaceMap[refFace.mNeighborFaceID[i]];

		if(f.ContainVertexID(iV1) && f.ContainVertexID(iV2))
			return f.mID;
	}
	return -1;
}

void iCathMesh::CaculateAdjacentByEdge( std::vector<std::vector<int> >& refEdgeAdj,const std::vector<int>& viTriangles )
{
	refEdgeAdj.clear();
	
	int iTriangle = viTriangles.size();

	if(iTriangle == 0)	return;

	refEdgeAdj.resize(iTriangle);

	for(int i = 0; i < iTriangle; ++i)
	{
		const Face& rFace = mFaceMap[viTriangles[i]];

		for(int j = 0; j < 3; ++j)
		{
			int iV1 = rFace.mVerticesID[j];
			int iV2 = rFace.mVerticesID[(j+1)%3];

			int iNeighborID = GetNeighborFaceBySharedEdge(viTriangles[i],iV1,iV2);

			assert(iNeighborID!=-1);
			
			if(iNeighborID == -1)
				printf("Fuck, It Equals With -1. %d.\n",viTriangles[i]);

			refEdgeAdj[i].push_back(iNeighborID);
		}
	}
}

void iCathMesh::GetFaceVerticesByIndex( int i_iIndex, iv::vec3& o_vP1,iv::vec3& o_vP2,iv::vec3& o_vP3 )
{
	FaceMap::reverse_iterator itr = mFaceMap.rbegin();
	if(i_iIndex >= itr->first)
		return;

	const Face& rFace = mFaceMap[i_iIndex];

	o_vP1 = mVertexMap[rFace.mVerticesID[0]].mPositions;
	o_vP2 = mVertexMap[rFace.mVerticesID[1]].mPositions;
	o_vP3 = mVertexMap[rFace.mVerticesID[2]].mPositions;
}

char iCathMesh::SphereVsTriangle( const iv::vec3 &vCenter,float fRadius,const iv::vec3 &v1,const iv::vec3 &v2,const iv::vec3 &v3 )
{
	iv::dvec3 dv1(v1.x,v1.y,v1.z);
	iv::dvec3 dv2(v2.x,v2.y,v2.z);
	iv::dvec3 dv3(v3.x,v3.y,v3.z);

	iv::dvec3 dvCenter(vCenter.x,vCenter.y,vCenter.z);

	double squareRadius = static_cast<double>(fRadius) * fRadius;
	double squareLength1 = iv::square(dv1-dvCenter);
	double squareLength2 = iv::square(dv2-dvCenter);
	double squareLength3 = iv::square(dv3-dvCenter);

	char retCode = 7;

	if( squareLength1 > squareRadius )
		retCode &= ~1;
	if( squareLength2 > squareRadius )
		retCode &= ~(1 << 1);
	if( squareLength3 > squareRadius )
		retCode &= ~(1 << 2);

	return retCode;
}

void iCathMesh::SetDirty()
{
	mbDirty = true;
}

bool iCathMesh::CheckAdjacentyComplete()
{
	printf("Checking Adjacency Information..................");
	vErrorID.clear();
	bool bResult = true;
	for(FaceMap::iterator itr = mFaceMap.begin(); itr!= mFaceMap.end(); ++itr)
	{
		const Face& rFace = itr->second;
		if(rFace.mNeighborFaceID.size()!=3)
		{
			m_bAdjacentyComplete = false;
			vErrorID.push_back(rFace.mID);
			m_iErrorIndex = rFace.mID;
			bResult = false;
		}
	}

	EdgeMap::iterator itr;
	for(itr = mEdges.begin(); itr != mEdges.end(); ++itr)
	{
		if(itr->second.size()!=2)
		{
			vErrorID.push_back(itr->first.iV1);
			vErrorID.push_back(itr->first.iV2);
		}
	}

	if(bResult)
		printf("Passed!\n");
	else
		printf("Failed!\n");

	return bResult;
}

int iCathMesh::GetErrorFaceID() const
{
	return m_iErrorIndex;
}

void iCathMesh::CorrectWinding( int iFace,const iv::vec3& refNormal )
{
	Face& rf = mFaceMap[iFace];

	int& i1 = rf.mVerticesID[0];
	int& i2 = rf.mVerticesID[1];
	int& i3 = rf.mVerticesID[2];

	iv::vec3 n = iv::normalize(iv::cross(mVertexMap[i3].mPositions - mVertexMap[i2].mPositions,mVertexMap[i1].mPositions - mVertexMap[i2].mPositions));

	if(iv::dot(n,refNormal) <= 0.0f)
	{
		int tmp = i1;
		i1 = i2;
		i2 = tmp;
	}
}

void iCathMesh::MakeThingsRight(std::vector<Face>& io_vFaces,std::vector<Vertex>& io_vVertex)
{
	io_vVertex.clear();
	io_vFaces.clear();

	std::map<int,int> mOld2New;
	std::map<int,int>::iterator itrX;

	for(FaceMap::iterator itr = mFaceMap.begin(); itr!=mFaceMap.end(); ++itr)
	{
		Face nf;
		nf.mID = io_vFaces.size();
		const Face& rFace = itr->second;
		for(int i = 0; i < 3; ++i)
		{
			int iV = rFace.mVerticesID[i];
			itrX = mOld2New.find(iV);

			// We've already fetched this vertex and modified it.
			// Fetch it.
			if(itrX!=mOld2New.end())
				nf.mVerticesID.push_back(itrX->second);
			else	// This is the first time that we fetch it.
			{
				Vertex nv = mVertexMap[iV];
				nv.mID = io_vVertex.size();
				io_vVertex.push_back(nv);
				nf.mVerticesID.push_back(nv.mID);
				mOld2New[iV] = nv.mID;
			}
		}
		io_vFaces.push_back(nf);
	}
}

void iCathMesh::SaveAsSTL( const char* pStrFilePath )
{
	std::vector<Face> vFace;
	std::vector<Vertex> vVertex;
	MakeThingsRight(vFace,vVertex);

	int faceCnt = vFace.size();

	std::vector<char> dataBuffer;

	int pSize = sizeof(iv::vec3);
	
	int bufferSize = 80 + 4 + (2 + 4 * pSize) * faceCnt + 4;
	dataBuffer.resize(bufferSize);
	char *pCur = &dataBuffer[0];
	pCur += 80;
	memcpy(pCur,&faceCnt,4);
	pCur += 4;
	
	for(int i = 0; i < faceCnt ; ++i )
	{
		const iCathMesh::Face& face = vFace[i];
		memcpy(pCur,&face.mNormal,pSize);
		pCur += pSize;
		for(int j = 0; j < 3; ++j )
		{
			memcpy(pCur,&vVertex[face.mVerticesID[j]].mPositions,pSize);
			pCur += pSize;
		}
		pCur += 2;
	}

	std::ofstream oFile(pStrFilePath,std::ios::binary);
	oFile.write(&dataBuffer[0],bufferSize);
	oFile.close();

	printf("Save Succeeded!\n");
}

void iCathMesh::SplitEdge( const Edge& rEdge,const Vertex& rVertex,int *o_iNewlyAddedFace /*= 0*/ )
{
	EdgeMap::iterator itr = mEdges.find(rEdge);
	if(itr == mEdges.end())
	{
		printf("Coundn't find this edge. @iCathMesh::SplitEdge.\n");
		return;
	}

	int iFace[2] = { -1,-1 };
	for(int i = 0; i < itr->second.size(); ++i)
		iFace[i] = itr->second[i];

	FaceMap::reverse_iterator itrX = mFaceMap.rbegin();

	int iNewFaces[4] = {
		itrX->first + 1,
		itrX->first + 2,
		itrX->first + 3,
		itrX->first + 4
	};
	// For The First Triangles Sharing This Edge.
	const Face& rFace = mFaceMap[iFace[0]];
	int iLonely = rFace.GetExclusiveVertexIDByEdge(rEdge);

	Edge e1(iLonely,rEdge.iV1),e2(iLonely,rEdge.iV2);

	int iNeighbor1 = GetNeighborFaceByEdge(iFace[0],e1);
	int iNeighbor2 = GetNeighborFaceByEdge(iFace[0],e2);

	Face newFace1;
	newFace1.mID = iNewFaces[0];
	newFace1.mVerticesID.push_back(iLonely);
	newFace1.mVerticesID.push_back(rEdge.iV1);
	newFace1.mVerticesID.push_back(rVertex.mID);
	newFace1.mNeighborFaceID.push_back(iNeighbor1);
	newFace1.mNeighborFaceID.push_back(iNewFaces[1]);
	newFace1.mNeighborFaceID.push_back(iNewFaces[2]);
	mFaceMap[newFace1.mID] = newFace1;
	CorrectWinding(newFace1.mID,rVertex.mNormals);

	Face& rfNeighbor1 = mFaceMap[iNeighbor1];
	for(int i = 0; i < rfNeighbor1.mNeighborFaceID.size(); ++i)
	{
		if(rfNeighbor1.mNeighborFaceID[i] == iFace[0])
			rfNeighbor1.mNeighborFaceID[i] = newFace1.mID;
	}
	for(int i = 0; i < mEdges[e1].size(); ++i)
	{
		if(mEdges[e1][i] == iFace[0])
			mEdges[e1][i] = newFace1.mID;
	}

	Face newFace2;
	newFace2.mID = iNewFaces[1];
	newFace2.mVerticesID.push_back(iLonely);
	newFace2.mVerticesID.push_back(rEdge.iV2);
	newFace2.mVerticesID.push_back(rVertex.mID);
	newFace2.mNeighborFaceID.push_back(iNeighbor2);
	newFace2.mNeighborFaceID.push_back(iNewFaces[0]);
	newFace2.mNeighborFaceID.push_back(iNewFaces[3]);
	mFaceMap[newFace2.mID] = newFace2;
	CorrectWinding(newFace2.mID,rVertex.mNormals);

	Face& rfNeighbor2 = mFaceMap[iNeighbor2];
	for(int i = 0; i < rfNeighbor2.mNeighborFaceID.size(); ++i)
	{
		if(rfNeighbor2.mNeighborFaceID[i] == iFace[0])
			rfNeighbor2.mNeighborFaceID[i] = newFace2.mID;
	}
	for(int i = 0; i < mEdges[e2].size(); ++i)
	{
		if(mEdges[e2][i] == iFace[0])
			mEdges[e2][i] = newFace2.mID;
	}

	// For The Second Triangles Sharing This Edge.
	const Face& rFace2 = mFaceMap[iFace[1]];
	int iLonely2 = rFace2.GetExclusiveVertexIDByEdge(rEdge);

	Edge e3(iLonely2,rEdge.iV1),e4(iLonely2,rEdge.iV2);

	int iNeighbor3 = GetNeighborFaceByEdge(iFace[1],e3);
	int iNeighbor4 = GetNeighborFaceByEdge(iFace[1],e4);

	Face newFace3;
	newFace3.mID = iNewFaces[2];
	newFace3.mVerticesID.push_back(iLonely2);
	newFace3.mVerticesID.push_back(rEdge.iV1);
	newFace3.mVerticesID.push_back(rVertex.mID);
	newFace3.mNeighborFaceID.push_back(iNeighbor3);
	newFace3.mNeighborFaceID.push_back(iNewFaces[3]);
	newFace3.mNeighborFaceID.push_back(iNewFaces[0]);
	mFaceMap[newFace3.mID] = newFace3;
	CorrectWinding(newFace3.mID,rVertex.mNormals);

	Face& rfNeighbor3 = mFaceMap[iNeighbor3];
	for(int i = 0; i < rfNeighbor3.mNeighborFaceID.size(); ++i)
	{
		if(rfNeighbor3.mNeighborFaceID[i] == iFace[1])
			rfNeighbor3.mNeighborFaceID[i] = newFace3.mID;
	}
	for(int i = 0; i < mEdges[e3].size(); ++i)
	{
		if(mEdges[e3][i] == iFace[1])
			mEdges[e3][i] = newFace3.mID;
	}

	Face newFace4;
	newFace4.mID = iNewFaces[3];
	newFace4.mVerticesID.push_back(iLonely2);
	newFace4.mVerticesID.push_back(rEdge.iV2);
	newFace4.mVerticesID.push_back(rVertex.mID);
	newFace4.mNeighborFaceID.push_back(iNeighbor4);
	newFace4.mNeighborFaceID.push_back(iNewFaces[1]);
	newFace4.mNeighborFaceID.push_back(iNewFaces[2]);
	mFaceMap[newFace4.mID] = newFace4;
	CorrectWinding(newFace4.mID,rVertex.mNormals);

	Face& rfNeighbor4 = mFaceMap[iNeighbor4];
	for(int i = 0; i < rfNeighbor4.mNeighborFaceID.size(); ++i)
	{
		if(rfNeighbor4.mNeighborFaceID[i] == iFace[1])
			rfNeighbor4.mNeighborFaceID[i] = newFace4.mID;
	}
	for(int i = 0; i < mEdges[e4].size(); ++i)
	{
		if(mEdges[e4][i] == iFace[1])
			mEdges[e4][i] = newFace4.mID;
	}

	// Add 4 New Edges.
	// Delete 1 Edge.
	// Delete 2 Faces.
	Edge ne1(rEdge.iV1,rVertex.mID),
		ne2(rEdge.iV2,rVertex.mID),
		ne3(iLonely,rVertex.mID),
		ne4(iLonely2,rVertex.mID);

	mEdges[ne1].push_back(iNewFaces[0]);
	mEdges[ne1].push_back(iNewFaces[2]);
	mEdges[ne2].push_back(iNewFaces[1]);
	mEdges[ne2].push_back(iNewFaces[3]);
	mEdges[ne3].push_back(newFace1.mID);
	mEdges[ne3].push_back(newFace2.mID);
	mEdges[ne4].push_back(newFace3.mID);
	mEdges[ne4].push_back(newFace4.mID);

	mEdges.erase(rEdge);
	mFaceMap.erase(iFace[0]);
	mFaceMap.erase(iFace[1]);

	if(o_iNewlyAddedFace)
	{
		for(int i = 0; i < 4; ++i)
			o_iNewlyAddedFace[i] = iNewFaces[i];
	}
	mbDirty = true;
}



int iCathMesh::GetNeighborFaceByEdge( int iHost,const Edge& rEdge )
{
	int retVal = -1;
	const Face& rFace = mFaceMap[iHost];
	EdgeMap::iterator itr = mEdges.find(rEdge);
	assert(itr!=mEdges.end());
	for(int i = 0; i < itr->second.size(); ++i)
	{
		if(itr->second[i] != iHost)
			retVal = itr->second[i];
	}
	return retVal;
}

void iCathMesh::InterpolateVertexAttrib( const Vertex& i_rV1,const Vertex& i_rV2,float fFactor,Vertex& o_rV )
{
	o_rV.mPositions = i_rV2.mPositions * fFactor + i_rV1.mPositions * (1.0f - fFactor);
	o_rV.mNormals= iv::normalize(i_rV2.mNormals * fFactor + i_rV1.mNormals * (1.0f - fFactor));
}

void iCathMesh::SphereVsLineSegment( const iv::vec3& vCenter,float fRadius,
	const iv::vec3& p1,const iv::vec3& p2,float* o_pFactor,int* io_iRootNum )
{
	double squareRadius = static_cast<double>(fRadius) * fRadius;

	iv::dvec3 m = iv::dvec3(p1.x,p1.y,p1.z) - iv::dvec3(vCenter.x,vCenter.y,vCenter.z);
	iv::dvec3 n = iv::dvec3(p2.x,p2.y,p2.z) - iv::dvec3(p1.x,p1.y,p1.z);

	double mn = iv::dot(m,n);

	double nSquare = iv::square(n);
	double nLength = iv::length(n);

	// delta = b^2 - 4ac;
	double delta = 4.0 * mn * mn - 4.0 * nSquare * (iv::square(m) - squareRadius);


	*io_iRootNum = 0;

	if(delta < 0.0)		return;

	double sqrtDelta = sqrt(delta);

	// t1 < t2
	double t1 = ( -2.0 * mn - sqrtDelta ) / (2.0 * nSquare);
	double t2 = ( -2.0 * mn + sqrtDelta ) / (2.0 * nSquare);

	if( t1 >= 0.0 && t1 <= 1.0)
		o_pFactor[(*io_iRootNum)++] = t1;
	if( t2 >= 0.0 && t2 <= 1.0)
		o_pFactor[(*io_iRootNum)++] = t2;
	if((*io_iRootNum) == 2)
		printf("Holy Fuck! Two Roots.******************************\n");
}

void iCathMesh::CreateTumour( float fRadius,int iSamples,float fSphereRadius,
	const iv::vec3& vCenter,const std::vector<int>& vThatCircle )
{
	int iThatSample = vThatCircle.size();
	std::vector<std::vector<int>> vPoints(iThatSample);
	m_TumorFace_Part = std::vector<IntArray>(iThatSample);

	iCathMesh::VertexMap::iterator itr;

	iv::vec3 vNormal;
	for(int i = 0; i < iThatSample; ++i)
	{
		itr = mVertexMap.find(vThatCircle[i]);
		if(itr!= mVertexMap.end())
		{
			vNormal += itr->second.mNormals;
		}
	}
	vNormal = iv::normalize(vNormal / static_cast<float>(iThatSample));

	float fUnder = sqrt(fRadius * fRadius -  fSphereRadius * fSphereRadius);

	printf("fRadius : %f. fSphereRadius : %f.\n",fRadius,fSphereRadius);
	printf("fUnder : %f.\n",fUnder);

	iv::vec3 vTumorCenter = vCenter + vNormal * (fUnder);

	m_vRecentTumorCenter = vTumorCenter;
	m_fRecentTumorRadius = fRadius;

	iv::vec3 vTopest = vCenter + vNormal * (fUnder + fRadius);

	iCathMesh::VertexMap::reverse_iterator itrX = mVertexMap.rbegin();

	// @nV is the toppest vertex of the sphere/tumor.
	iCathMesh::Vertex nV;
	nV.mID = itrX->first + 1;
	nV.mPositions = vTopest;
	nV.mNormals = vNormal;
	mVertexMap[nV.mID] = nV;


	// Calculate All Sample Points.
	// Fetch A Vertex From vThatCircle, We Call It vStart.
	// Get The vTopest, We Call It vEnd.
	// Get The vTumorCenter.
	// Get The Sample Num.
	iv::vec3 vEnd = vTopest - vTumorCenter;
	float vEndLen = iv::length(vEnd);
	for(int j = 0; j < iThatSample; ++j)
	{
		iv::vec3 vStart = iv::normalize(mVertexMap[vThatCircle[j]].mPositions - vTumorCenter) * vEndLen;
		mVertexMap[vThatCircle[j]].mPositions = vTumorCenter + vStart;

		float fRadianRange = iv::GetRadianBetween(vStart,vEnd);
		float fStepRadian = fRadianRange / static_cast<float>(iSamples);
		iv::vec3 rotAxis = iv::normalize(iv::cross(vStart,vEnd));

		// Here, We Added The First One.
		vPoints[j].push_back(vThatCircle[j]);

		for(int i = 1; i < iSamples; ++i)
		{
			float rotRadian = static_cast<float>(i) * fStepRadian;
			iv::mat3 rotMat = iv::mat3(iv::rotate(rotRadian,rotAxis));
			iv::vec3 thisFuck = rotMat * vStart;
			iv::vec3 newp = vTumorCenter + thisFuck;

			iCathMesh::Vertex newV;
			iCathMesh::VertexMap::reverse_iterator itrZ = mVertexMap.rbegin();
			newV.mID = itrZ->first + 1;
			newV.mPositions = newp;
			newV.mNormals = iv::normalize(thisFuck);
			mVertexMap[newV.mID] = newV;
			// Here, We Added The Newly Added.
			vPoints[j].push_back(newV.mID);
		}

		// Add The Top One.
		vPoints[j].push_back(nV.mID);
	}


	

	// Create Faces.
	std::vector<int> vTumorFace;

	int iX = vPoints.size();
	for(int i = 0 ; i < iX; ++i)
	{
		const std::vector<int>& v1 = vPoints[i];
		const std::vector<int>& v2 = vPoints[(i+1)%iX];
		int iY = vPoints[i].size();
		for(int j = 0; j < iY - 1; ++j)
		{
			if( j == iY - 2)
			{
				iCathMesh::FaceMap::reverse_iterator itr = mFaceMap.rbegin();
				iCathMesh::Face newFace1;
				newFace1.mID = itr->first + 1;
				newFace1.mVerticesID.push_back(v1[j]);
				newFace1.mVerticesID.push_back(v1[j+1]);
				newFace1.mVerticesID.push_back(v2[j]);
				mFaceMap[newFace1.mID] = newFace1;
				iv::vec3 vRefNormal = mVertexMap[v1[j+1]].mNormals;
				CorrectWinding(newFace1.mID,vRefNormal);
				
				// Collect Tumor Face
				vTumorFace.push_back(newFace1.mID);
				m_TumorFace_Part[i].push_back(newFace1.mID);
			}
			else 
			{
				iCathMesh::FaceMap::reverse_iterator itr = mFaceMap.rbegin();
				iCathMesh::Face newFace1;
				newFace1.mID = itr->first + 1;
				newFace1.mVerticesID.push_back(v1[j]);
				newFace1.mVerticesID.push_back(v1[j+1]);
				newFace1.mVerticesID.push_back(v2[j]);
				mFaceMap[newFace1.mID] = newFace1;
				iv::vec3 vRefNormal = mVertexMap[v1[j+1]].mNormals;
				CorrectWinding(newFace1.mID,vRefNormal);
				// Collect Tumor Face
				vTumorFace.push_back(newFace1.mID);
				m_TumorFace_Part[i].push_back(newFace1.mID);

				iCathMesh::Face newFace2;
				newFace2.mID = newFace1.mID + 1;
				newFace2.mVerticesID.push_back(v1[j+1]);
				newFace2.mVerticesID.push_back(v2[j]);
				newFace2.mVerticesID.push_back(v2[j+1]);
				mFaceMap[newFace2.mID] = newFace2;
				CorrectWinding(newFace2.mID,vRefNormal);
				// Collect Tumor Face
				vTumorFace.push_back(newFace2.mID);
				m_TumorFace_Part[i].push_back(newFace2.mID);
			}
		}
	}

	if(!vTumorFace.empty())
	{
		FaceSet fs;
		for(int i = 0; i < vTumorFace.size(); ++i)
			fs.insert(vTumorFace[i]);

		TumorInfo tumorInfo;
		tumorInfo.eValid = TUMOR_VALID;
		tumorInfo.iFaceCount = fs.size();
		tumorInfo.vCenter = vTumorCenter;
		tumorInfo.fRadius = fRadius;
		mTumorInfos.push_back(tumorInfo);
		mTumors.push_back(fs);
	}

	m_TumorFace.clear();
	m_TumorFace = vTumorFace;
	
	
		
}

void iCathMesh::UpdateCollider()
{
	m_pCollider.reset();
	m_pCollider = std::shared_ptr<CxCollisionTest>(new CxCollisionTest);
	m_pCollider->Init(this);
}


std::shared_ptr<iCathMesh> iCathMesh::SeperateTumor( const iv::vec3& vCenter,float fRadius )
{
	std::vector<int> vCollideFace;

	m_pCollider->GetCollisionTriangleIDWithSphere(vCenter,fRadius,vCollideFace);

	if(vCollideFace.empty())
		return nullptr;

	return SeperateBumpMesh(vCollideFace[0]);
}



void iCathMesh::OptimizedWayToGrowTumor( const iv::vec3& vCenter,float fRadius )
{
	std::cout << "OptimizedWayToGrowTumor()" << std::endl;

	// Record backup mesh
	mFaceMapBackup.push_back(mFaceMap);
	mVertexMapBackup.push_back(mVertexMap);

	//fRadius *= 2;
	mVesselRadius = fRadius*2.0f;
	
	std::vector<int> vCollideFace;
	
	getMoreFace(vCenter,fRadius*2.0f ,vCollideFace);
	
	std::vector<int> vInside;
	std::vector<int> vIntersect;
	std::vector<int> vOutside;
	std::vector<int> vNewlyAdded;

	for(int i = 0; i < vCollideFace.size(); ++i)
	{
		int iFaceID = vCollideFace[i];

		const Face& crf = mFaceMap[iFaceID];

		iv::vec3 v1,v2,v3;

		GetFaceVerticesByIndex(iFaceID,v1,v2,v3);

		char retChar = SphereVsTriangle(vCenter,fRadius,v1,v2,v3);

		if(retChar == 0)
			vOutside.push_back(iFaceID);
		else if(retChar == 7)
			vInside.push_back(iFaceID);
		else
			vIntersect.push_back(iFaceID);
	}

	printf("%d.OutSide.\n",vOutside.size());

	// Process Outside Array

	std::map<Edge,Vertex> mSplits;
	std::map<Edge,Vertex>::iterator itr;

	for(int i = 0; i < vOutside.size(); ++i)
	{
		int iFaceID = vOutside[i];
		const Face& rFace = mFaceMap[iFaceID];
		for(int j = 0; j < 3; ++j)
		{
			int iV1 = rFace.mVerticesID[j];
			int iV2 = rFace.mVerticesID[(j+1)%3];

			const Vertex& rV1 = mVertexMap[iV1];
			const Vertex& rV2 = mVertexMap[iV2];

			float factors[2];
			int iRoots = 0;
			SphereVsLineSegment(vCenter,fRadius,rV1.mPositions,rV2.mPositions,factors,&iRoots);
			if(iRoots == 2)
			{
				float ft = (factors[0] + factors[1]) * 0.5f;
				Vertex nv;
				nv.mID = GenNextNewVertexID();
				InterpolateVertexAttrib(rV1,rV2,ft,nv);
				mVertexMap[nv.mID] = nv;

				Edge e(iV1,iV2);

				itr = mSplits.find(e);
				if(itr == mSplits.end())
					mSplits[e] = nv;
			}
		}
	}

	printf("%d Need Split.\n",mSplits.size());
	for(itr = mSplits.begin(); itr != mSplits.end(); ++itr)
	{
		int vNewly[4];
		SplitEdge(itr->first,itr->second,vNewly);
		for(int i = 0; i < 4; ++i)
			vNewlyAdded.push_back(vNewly[i]);
	}

	bool bAdjInfoCompletion = CheckAdjacentyComplete();

	printf("Newly Added : ");
	for(int i = 0; i < vNewlyAdded.size(); ++i)
		printf("\t%d.",vNewlyAdded[i]);

	std::vector<int> vActualIntersect;

	// Gather All Intersect Face.
	FaceMap::iterator itrf;
	for(int i = 0 ; i < vIntersect.size(); ++i)
	{
		int iID = vIntersect[i];
		itrf = mFaceMap.find(iID);
		if(itrf == mFaceMap.end())
			printf("This Face Has Been Deleted When Split Edge.\n");
		else
			vActualIntersect.push_back(iID);
	}
	for(int i = 0 ; i < vNewlyAdded.size(); ++i)
	{
		int iID = vNewlyAdded[i];
		itrf = mFaceMap.find(iID);
		if(itrf == mFaceMap.end())
			printf("This Newly Added Face #%d Has Been Deleted When Split Edge.\n",iID);
		else
			vActualIntersect.push_back(iID);
	}

	std::map<Edge,Vertex> mEdge2Vertex;
	bool bIntersectCheck = CheckAllEdgeWithOneIntersection(vCenter,fRadius,vActualIntersect,mEdge2Vertex);
	
	if (mEdge2Vertex.size() != vActualIntersect.size())
	{
	}

	SortFaceIDs_Opted(vActualIntersect,mEdge2Vertex);

	CreateIntersectionEdgeWithOutDuplication_Opted(vCenter,fRadius,vActualIntersect,vInside,mEdge2Vertex);

	InValidAdjacencyInfo();

	GenAdjacency();
	
	//drawTriangles(m_TumorFace);
	CalcPerFaceGravity();
	SetDirty();	
	
	DrawRandomBending();
	MeshSmothing();
	
	MeshSimplification();
	MeshOptimization();	

	CalcPerTumorFaceNormal();	
	CalcPerTumorVertexNormal();


	/*IntArray a1;
	getMoreNeighber(m_TumorFace, a1);
	drawTriangles(a1);*/
}

void iCathMesh::MeshSimplification()
{
	
	MeshSimplification(m_TumorFace);

	
}
void iCathMesh::MeshSmothing()
{
	std::cout << "## MeshSmothing" << std::endl;
	IntArray moreface, lessface;
	getMoreNeighber(m_TumorFace, moreface);
	getMoreNeighber(moreface, moreface);
	//getMoreNeighber(moreface, moreface);
	get_difference(moreface, m_TumorFace, lessface);

	for (int i = 0; i < m_TumorFace_Part.size(); i++)
	{
		for (int j = 0; j < 6; j++)
		{
			lessface.push_back(m_TumorFace_Part[i][j]);
		}
	}
	MeshSmothing(lessface, 400);
	MeshSmothing(moreface, 2);

}


void iCathMesh::MeshOptimization()
{
	IntArray more_face, fixed_face, fixed_vertex,less_face;
	getMoreNeighber(m_TumorFace, more_face);
	getMoreNeighber(more_face, more_face);
	getMoreNeighber(more_face, more_face);

	for (int i = 0; i < 30; i++)
	{
		fixed_face.push_back(m_TumorFace[getRandom(0, m_TumorFace.size() / 3)]);
	}
	for (int i = 0; i < 30; i++)
	{
		fixed_face.push_back(m_TumorFace[getRandom(m_TumorFace.size() / 3 + 1, 2 * m_TumorFace.size() / 3)]);
	}
	for (int i = 0; i < 30; i++)
	{
		fixed_face.push_back(m_TumorFace[getRandom(2 * m_TumorFace.size() / 3 + 1, m_TumorFace.size())]);
	}

	getVIDByFID(fixed_face, fixed_vertex);
	MeshOptimization(m_TumorFace, fixed_vertex);	
	
	MeshSmothing(more_face, 1);
	
}

void iCathMesh::get_difference(IntArray faces_id_large, IntArray faces_id_small, IntArray& faces_id_result)
{
	//facesIDResult.resize(facesIDBig.size() - facesIDSmall.size());
	faces_id_result.clear();
	for (int i = 0 ; i < faces_id_large.size(); i++)
	{
		std::vector <int>::iterator iElementFirst = find(faces_id_small.begin(),
			faces_id_small.end(), faces_id_large[i]);
		if (iElementFirst == faces_id_small.end())//如果facesIDBig中这个元素没有在facesIDSmall中出现
		{
			faces_id_result.push_back(faces_id_large[i]);
		}
	}
}

int iCathMesh::GenNextNewVertexID()
{
	if(mVertexMap.size() == 0)
		return 0;
	VertexMap::reverse_iterator itr = mVertexMap.rbegin();
	return itr->first + 1;
}

bool iCathMesh::SortFaceIDs_Opted( std::vector<int> &io_vIDs,const std::map<Edge,Vertex>& mEdge2Vertex )
{
	if(io_vIDs.empty())	return false;

	std::set<int> sFace;
	
	std::vector<int> vResult;

	for(int i = 0; i < io_vIDs.size(); ++i)
		sFace.insert(io_vIDs[i]);

	const Face& rFace = mFaceMap[io_vIDs[0]];

	std::map<Edge,Vertex>::const_iterator itr;

	Edge startEdge(-1,-1);
	int iStartFace = -1;
	Edge endEdge(-1,-1);

	for(int i = 0; i < 3 ; ++i)
	{
		int iV1 = rFace.mVerticesID[i];
		int iV2 = rFace.mVerticesID[(i+1)%3];
		
		itr = mEdge2Vertex.find(Edge(iV1,iV2));

		if(itr != mEdge2Vertex.end())
		{
			startEdge = itr->first;
			break;
		}
	}

	vResult.push_back(io_vIDs[0]);
	sFace.erase(io_vIDs[0]);
	iStartFace = io_vIDs[0];

	while(!sFace.empty())
	{
		int iNeighbor = GetNeighborFaceByEdge(iStartFace,startEdge);
		const Face& rf = mFaceMap[iNeighbor];
		for(int j = 0; j < 3; ++j)
		{
			int iV1 = rf.mVerticesID[j];
			int iV2 = rf.mVerticesID[(j+1)%3];
			Edge e(iV1,iV2);
			itr = mEdge2Vertex.find(e);
			if(itr!=mEdge2Vertex.end() && !(e == startEdge))
			{
				startEdge = e;
				iStartFace = iNeighbor;
				vResult.push_back(iStartFace);
				sFace.erase(iStartFace);
				break;
			}
		}
	}

	io_vIDs = vResult;

	return true;

}

bool iCathMesh::CheckAllEdgeWithOneIntersection(const iv::vec3& vCenter,float fRadius, 
	const std::vector<int>& vTriangles,std::map<Edge,Vertex>& mEdge2Vertex )
{
	mEdge2Vertex.clear();
	std::map<Edge,Vertex>::iterator itr;

	printf("Checking Edge Intersection..................");
	bool retVal = true;
	for(int i = 0; i < vTriangles.size(); ++i)
	{
		int iTotalRoots = 0;
		int iFaceID = vTriangles[i];
		const Face& rFace = mFaceMap[iFaceID];
		for(int j = 0; j < 3; ++j)
		{
			int iV1 = rFace.mVerticesID[j];
			int iV2 = rFace.mVerticesID[(j+1)%3];

			const Vertex& rV1 = mVertexMap[iV1];
			const Vertex& rV2 = mVertexMap[iV2];

			float factors[2];
			int iRoots = 0;
			SphereVsLineSegment(vCenter,fRadius,rV1.mPositions,rV2.mPositions,factors,&iRoots);

			// iRoots Must Equals 1!!!!!
			if( iRoots == 1)
			{
				float ft = factors[0];
				Vertex nv;
				nv.mID = GenNextNewVertexID();
				InterpolateVertexAttrib(rV1,rV2,ft,nv);
				mVertexMap[nv.mID] = nv;

				Edge e(iV1,iV2);

				itr = mEdge2Vertex.find(e);
				if(itr == mEdge2Vertex.end())
					mEdge2Vertex[e] = nv;
			}
			
			if(iRoots > 1)
				retVal = false;

			iTotalRoots += iRoots;
		}

		if(iTotalRoots != 2)
			retVal = false;
	}

	if(retVal)
		printf("Sucess!\n");
	else
		printf("Failed!\n");

	

	return retVal;
}

void iCathMesh::CreateIntersectionEdgeWithOutDuplication_Opted( const iv::vec3& vCenter,
	float fRadius,const std::vector<int>& vIntersectFaceID,std::vector<int>& vFaceToDelete,
	const std::map<Edge,Vertex>& mEdge2Vertex )
{
	int iFaceNum = vIntersectFaceID.size();

	if(iFaceNum == 0)	return;

	std::vector<int> vCircleVertexID;

	std::map<Edge,Vertex>::const_iterator itr;

	// For Every Intersected Face, We Split It Into 4 Faces.
	// Attention! Every Time We Modify The Original Mesh, Adjacency Info. Can Be Invalid.
	// We Better Keep The Adjacency Info. Valid As Possible As We Can.
	for(int i = 0; i < iFaceNum ; ++i)
	{
		std::vector<int> vInside;
		std::vector<int> vOutside;

		std::vector<int> vNewV;

		iCathMesh::Face& rFace = mFaceMap[vIntersectFaceID[i]];

		const Face& crf = mFaceMap[vIntersectFaceID[i]];

		iv::vec3 v1,v2,v3;

		GetFaceVerticesByIndex(vIntersectFaceID[i],v1,v2,v3);

		char xCode = SphereVsTriangle(vCenter,fRadius,v1,v2,v3);

		for(int j = 0; j < 3; ++j)
		{
			int iV1 = rFace.mVerticesID[j];
			int iV2 = rFace.mVerticesID[(j+1)%3];

			Edge e(iV1,iV2);

			itr = mEdge2Vertex.find(e);

			// Found It!
			if(itr != mEdge2Vertex.end())
			{
				vInside.push_back(itr->second.mID);
				vOutside.push_back(itr->second.mID);
				vNewV.push_back(itr->second.mID);
			}

			if((xCode >> ((j+1)%3)) & 0x1)	// The Edge End Point Inside Sphere.
				vInside.push_back(iV2);
			else
				vOutside.push_back(iV2);
		}



		// After The Optimization, Its Size === 2!
		// Next, We Add Vertices To vCircleVertexID In The Right Order.
		if(vCircleVertexID.size()<2)
		{
			vCircleVertexID.push_back(vNewV[0]);
			vCircleVertexID.push_back(vNewV[1]);
		}
		else
		{
			int iSize = vCircleVertexID.size();
			if(vNewV[0] != vCircleVertexID[iSize - 1] && vNewV[1] != vCircleVertexID[iSize-1])
			{
				int tmp = vCircleVertexID[iSize - 1];
				vCircleVertexID[iSize - 1] = vCircleVertexID[iSize - 2];
				vCircleVertexID[iSize - 2] = tmp;
			}

			if(vNewV[0] == vCircleVertexID[iSize - 1])
				vCircleVertexID.push_back(vNewV[1]);
			else
				vCircleVertexID.push_back(vNewV[0]);
		}



		// Next, We Recreate Triangels.
		if(vInside.size() == 3)
		{
			// Inside First
			for(int k = 0; k < vInside.size(); ++k)
				rFace.mVerticesID[k] = vInside[k];
			vFaceToDelete.push_back(rFace.mID);

			iCathMesh::FaceMap::reverse_iterator itr = mFaceMap.rbegin();
			// Outside Next.
			iCathMesh::Face newFace1;
			newFace1.mID = itr->first + 1;
			newFace1.mVerticesID.push_back(vOutside[0]);
			newFace1.mVerticesID.push_back(vOutside[1]);
			newFace1.mVerticesID.push_back(vOutside[2]);
			mFaceMap[newFace1.mID] = newFace1;

			iCathMesh::Face newFace2;
			newFace2.mID = newFace1.mID + 1;
			newFace2.mVerticesID.push_back(vOutside[0]);
			newFace2.mVerticesID.push_back(vOutside[2]);
			newFace2.mVerticesID.push_back(vOutside[3]);
			mFaceMap[newFace2.mID] = newFace2;
		}

		if(vOutside.size() == 3)
		{
			// OutSide First.
			for(int k = 0; k < vOutside.size(); ++k)
				rFace.mVerticesID[k] = vOutside[k];

			iCathMesh::FaceMap::reverse_iterator itr = mFaceMap.rbegin();
			// Inside Next.
			iCathMesh::Face newFace1;
			newFace1.mID = itr->first + 1;
			newFace1.mVerticesID.push_back(vInside[0]);
			newFace1.mVerticesID.push_back(vInside[1]);
			newFace1.mVerticesID.push_back(vInside[2]);
			mFaceMap[newFace1.mID] = newFace1;
			vFaceToDelete.push_back(newFace1.mID);

			iCathMesh::Face newFace2;
			newFace2.mID = newFace1.mID + 1;
			newFace2.mVerticesID.push_back(vInside[0]);
			newFace2.mVerticesID.push_back(vInside[2]);
			newFace2.mVerticesID.push_back(vInside[3]);
			mFaceMap[newFace2.mID] = newFace2;
			vFaceToDelete.push_back(newFace2.mID);
		}
		vNewV.clear();
	}

	vCircleVertexID.pop_back();

	// Average Circle Points.
	for(int i = 0; i < 2; ++i)
		AverageSamplePoints(vCenter,fRadius,vCircleVertexID);

	// Increase Sample Number If Necessary.

	
	// Create A Hole.
	for(int i = 0; i < vFaceToDelete.size(); ++i)
	{
		int iID = vFaceToDelete[i];
		mFaceMap.erase(iID);
	}
	

	//TODO 第一个参数和第三个参数的比值可以决定肿瘤的突起程度
	CreateTumour(2.5f*fRadius,20, fRadius,vCenter,vCircleVertexID);

	InValidAdjacencyInfo();
	GenAdjacency();
	CalcPerFaceGravity();
	CalcPerTumorFaceNormal();
	CalcPerTumorVertexNormal();

	SetDirty();
}



void iCathMesh::AverageSamplePoints(const iv::vec3& vCenter,float fRadius,const std::vector<int> &vCircle )
{
	int iSize = vCircle.size();
	for(int i = 0; i < iSize; ++i)
	{
		const Vertex& rLeft = mVertexMap[vCircle[(i+iSize-1)%iSize]];
		const Vertex& rRight = mVertexMap[vCircle[(i+1)%iSize]];
		Vertex& rTarget = mVertexMap[vCircle[i]];
		rTarget.mPositions = (rLeft.mPositions + rRight.mPositions) * 0.5f;
		iv::vec3 dir = iv::normalize(rTarget.mPositions - vCenter);
		rTarget.mPositions = vCenter + dir * fRadius;
	}
}

void iCathMesh::CalcPerFaceNormal()
{
	printf("Calculating Face Normal........................");
	FaceMap::iterator itr;
	for(itr = mFaceMap.begin(); itr != mFaceMap.end(); ++itr)
	{
		Face& rFace = itr->second;
		
		iv::vec3 v1 = mVertexMap[rFace.mVerticesID[0]].mPositions;
		iv::vec3 v2 = mVertexMap[rFace.mVerticesID[1]].mPositions;
		iv::vec3 v3 = mVertexMap[rFace.mVerticesID[2]].mPositions;

		rFace.mNormal = iv::normalize(iv::cross(v3 - v2,v1 - v2));
	}
	printf("Done.\n");
}

void iCathMesh::CalcPerFaceGravity()
{
	printf("Calculating Face Gravity........................");
	FaceMap::iterator itr;
	for (itr = mFaceMap.begin(); itr != mFaceMap.end(); ++itr)
	{
		Face& rFace = itr->second;

		iv::vec3 v1 = mVertexMap[rFace.mVerticesID[0]].mPositions;
		iv::vec3 v2 = mVertexMap[rFace.mVerticesID[1]].mPositions;
		iv::vec3 v3 = mVertexMap[rFace.mVerticesID[2]].mPositions;

		float x = (v1.x + v2.x + v3.x) / 3.0;
		float y = (v1.y + v2.y + v3.y) / 3.0;
		float z = (v1.z + v2.z + v3.z) / 3.0;

		rFace.mGravity = iv::vec3(x, y, z);
	}
	printf("Done.\n");
}

std::shared_ptr<iCathMesh> iCathMesh::SeperateBumpMesh(int iAnyFaceOnTumor)
{
	int iFace = mFaceMap.size();
	bool *bExclusive = new bool[iFace];
	bool *bAdded = new bool[iFace];
	bool *bLastRound = new bool[iFace];

	memset(bExclusive,0,iFace * sizeof(bool));
	memset(bAdded,0,iFace * sizeof(bool));
	memset(bLastRound,0,iFace * sizeof(bool));

	std::vector<int> vNextRound;
	std::vector<int> vTumor;
	vNextRound.push_back(iAnyFaceOnTumor);
	vTumor.push_back(iAnyFaceOnTumor);
	int iIteration = 0;
	while( !vNextRound.empty() && iIteration < 50)
	{
		std::vector<int> vThisRound(vNextRound);
		vNextRound.clear();
		for(int i = 0; i < vThisRound.size(); ++i)
		{
			int iID = vThisRound[i];
			const iCathMesh::Face& rFace = mFaceMap[iID];
			iv::vec3 vThisNormal = rFace.mNormal;
			for(int j = 0; j < 3; ++j)
			{
				int iNeighbor = rFace.mNeighborFaceID[j];

				if(bLastRound[iNeighbor] || bAdded[iNeighbor] || bExclusive[iNeighbor])
					continue;

				const iCathMesh::Face& rf = mFaceMap[iNeighbor];
				iv::vec3 vNeighborNormal = rf.mNormal;
				float dotp = iv::dot(vThisNormal,vNeighborNormal);
				if( dotp < 0.1f)
				{
					printf("Skiped One.\n");
					bExclusive[iNeighbor] = true;
				}
				else
				{
					vNextRound.push_back(iNeighbor);
					bAdded[iNeighbor] = true;
					vTumor.push_back(iNeighbor);
				}
			}
			bLastRound[iID] = true;
		}
		++iIteration;
	}

	if(vNextRound.empty())
		printf("Stopped Becuse vNextRound Is Empty!\n");
	else
		printf("Iteration Time Up.\n");

	delete[] bExclusive;
	delete[] bAdded;
	delete[] bLastRound;

	if(mTumors.empty())
	{
		FaceSet fs;
		for(int i = 0; i < vTumor.size(); ++i)
			fs.insert(vTumor[i]);
		mTumors.push_back(fs);
	}

	return CreateSubMesh(vTumor);
}

void iCathMesh::NarrowMesh(int startId,int endId,std::shared_ptr<iCathMesh> *o_pNarrowedMesh/* = nullptr */)
{
	std::vector<std::vector<int> > vNarrowFaceIDs;
	GetNarrowedFaces(startId,endId,vNarrowFaceIDs);

	if(o_pNarrowedMesh)
	{
		std::vector<int> vSubMeshFace;
		for(int i = 0; i < vNarrowFaceIDs.size(); ++i)
		{
			for(int j = 0; j < vNarrowFaceIDs[i].size(); ++j)
				vSubMeshFace.push_back(vNarrowFaceIDs[i][j]);
		}
		*o_pNarrowedMesh = CreateSubMesh(vSubMeshFace);
	}

	MoveFaceToCenter(vNarrowFaceIDs);
}

void iCathMesh::NarrowMesh( const iv::vec3& vPick1,const iv::vec3& vPick2,
	std::shared_ptr<iCathMesh> *o_pNarrowedMesh /*= nullptr*/ )
{
	clearSelectedFace();
	UpdateCollider();
	if (!m_pCollider)
	{
		printf("Failed to Narrow Mesh..........Collider Not Initialized.\n");
		return;
	}

	std::vector<int> vCollideFace1, vCollideFace2;
	
	// TODO 第二个参数越小，选则的面越精准，但是如果过小在面比较稀疏的网格中会选不到
	getMoreFace(vPick1, 0.008f, vCollideFace1);	
	getMoreFace(vPick2, 0.008f, vCollideFace2);


	if (vCollideFace1.empty() || vCollideFace2.empty())
	{
		printf("Failed to Narrow Mesh..........Invalid Pick.\n");
		return;
	}

	printf("<%d,%d>\n", vCollideFace1[0], vCollideFace2[0]);

	std::vector<std::vector<int> > vNarrowFaceIDs;

	
	GetNarrowedFaces(vCollideFace1[0], vCollideFace2[0], vNarrowFaceIDs);


	// Calculate Central Line.
	int iCircleCount = vNarrowFaceIDs.size();	
	
	std::vector<iv::vec3> vMid;
	
	IntArray a1 = { 0,iCircleCount - 1 };

	for (int i = 0; i < 2; ++i)
	{
		iv::vec3 c(0.0f);
		for (int j = 0; j < vNarrowFaceIDs[a1[i]].size(); ++j)
		{
			const Face& rf = mFaceMap[vNarrowFaceIDs[a1[i]][j]];
			for (int k = 0; k < 3; ++k)
				c += mVertexMap[rf.mVerticesID[k]].mPositions;
		}
		c /= static_cast<float>(vNarrowFaceIDs[a1[i]].size() * 3);
		vMid.push_back(c);
	}
	for (int i = 0; i < vNarrowFaceIDs.size(); i++)
	{
		for (int j = 0; j < vNarrowFaceIDs[i].size(); j++)
		{
			face_ids_control.push_back(vNarrowFaceIDs[i][j]);
		}

	}

	if (face_ids_control.size() > 900)
	{
		return;
	}
	drawTriangles(face_ids_control);

	iv::vec3 p = (vPick1 + vPick2) / 2.0f;
	mLengthBetweenTwoVPick = iv::length(vPick2 - vPick1);	
	
	mVesselRadius = getVesselRadius(p, 0.02);
	sum_b = 0.0f;
	getMoreFace(p, mLengthBetweenTwoVPick*2.5f, face_ids_free);
	getMoreFace(p, mLengthBetweenTwoVPick*3.0f, face_ids_all);

	drawTriangles(face_ids_control);
	
}

bool iCathMesh::IsFaceCrossPlane( int iFaceID,const iv::vec3 &p,const iv::vec3 &d )
{
	iv::vec3 v1,v2,v3;
	GetFaceVerticesByIndex(iFaceID,v1,v2,v3);

	float p1 = iv::dot(v1-p,d);
	float p2 = iv::dot(v2-p,d);
	float p3 = iv::dot(v3-p,d);
	
	if( p1 > 0.0f && p2 > 0.0f && p3 > 0.0f )
		return false;
	else if( p1 < 0.0f && p2 < 0.0f && p3 < 0.0f )
		return false;
	else
		return true;
}

std::shared_ptr<iCathMesh> iCathMesh::CreateSubMesh( const std::vector<int>& vSubset )
{
	std::shared_ptr<iCathMesh> pTumorMesh(new iCathMesh);
	std::map<int,int> mOld2New;
	std::map<int,int>::iterator itrX;

	int iTumorFace = vSubset.size();

	for(int i = 0; i < iTumorFace; ++i)
	{
		const Face& crf = mFaceMap[vSubset[i]];
		Face nf;
		nf.mID = i;
		for(int j = 0; j < crf.mVerticesID.size(); ++j)
		{
			int iV = crf.mVerticesID[j];
			itrX = mOld2New.find(iV);
			if(itrX != mOld2New.end())
			{
				nf.mVerticesID.push_back(itrX->second);
			}
			else
			{
				Vertex nv = mVertexMap[iV];
				nv.mID = pTumorMesh->GenNextNewVertexID();
				pTumorMesh->mVertexMap[nv.mID] = nv;
				mOld2New[iV] = nv.mID;
				nf.mVerticesID.push_back(nv.mID);
			}
		}
		pTumorMesh->mFaceMap[nf.mID] = nf;
	}

	return pTumorMesh;
}

void iCathMesh::GetTumorSubMeshes( std::vector<std::shared_ptr<iCathMesh> >& vTumorMeshes, 
	bool bFillHole /* = false */ )
{
	vTumorMeshes.clear();

	for(int i = 0; i < mTumors.size(); ++i)
	{
		std::vector<int> vTumorFace;
		FaceSet::iterator itr;
		for(itr = mTumors[i].begin(); itr != mTumors[i].end(); ++itr)
			vTumorFace.push_back(*itr);
		std::shared_ptr<iCathMesh> pTumorMesh = CreateSubMesh(vTumorFace);
		vTumorMeshes.push_back(pTumorMesh);
		
		if(bFillHole)
			pTumorMesh->FillHole();
	}
}

bool iCathMesh::IsOnlyOneHole(std::vector<int>* pVertices /*= 0*/)
{
	GenEdgeAdjacency();

	std::set<Edge> vEdges;
	std::set<Edge>::iterator sitr;
	EdgeMap::iterator itr;
	for(itr = mEdges.begin(); itr != mEdges.end(); ++itr)
	{
		if(itr->second.size() != 2)
			vEdges.insert(itr->first);
		else if(itr->second.size() > 2)
			printf("More Than 2 Face Share An Edge!\n");
		else if(itr->second.size() == 0)
			printf("Ghost Face!\n");
	}

	if(vEdges.empty())
		return false;

	sitr = vEdges.begin();
	
	std::vector<int> opt_result;

	int iStopFlag = (*sitr).iV1;
	int iStart = (*sitr).iV2;

	opt_result.push_back(iStart);

	vEdges.erase(sitr);

	while(!vEdges.empty())
	{
		for(sitr = vEdges.begin(); sitr != vEdges.end(); ++sitr)
		{
			if((*sitr).iV1 == iStart || (*sitr).iV2 == iStart)
				break;
		}
		if(sitr == vEdges.end())
			return false;
		iStart = (*sitr).iV1 == iStart ? (*sitr).iV2 : (*sitr).iV1;

		opt_result.push_back(iStart);
		
		vEdges.erase(sitr);
	}

	if(iStopFlag == iStart)
	{
		opt_result.push_back(iStart);

		if(pVertices)
			*pVertices = opt_result;
		return true;
	}
	else
		 return false;

}

void iCathMesh::GenEdgeAdjacency()
{
	if(m_bEdgeAdjacency)
		return;

	mEdges.clear();

	for(FaceMap::iterator itr = mFaceMap.begin(); itr!=mFaceMap.end(); ++itr)
	{
		const Face& rFace = itr->second;
		for(int j = 0; j < 3; ++j)
		{
			int iV1 = rFace.mVerticesID[j];
			int iV2 = rFace.mVerticesID[(j+1)%3];
			mEdges[Edge(iV1,iV2)].push_back(rFace.mID);
		}
	}
	m_bEdgeAdjacency = true;
}

void iCathMesh::GenFaceAdjacency()
{
	if(m_bFaceAdjacency)
		return;

	for(FaceMap::iterator itr = mFaceMap.begin(); itr!=mFaceMap.end(); ++itr)
		itr->second.mNeighborFaceID.clear();
	
	GenEdgeAdjacency();

	for(FaceMap::iterator itr = mFaceMap.begin(); itr!=mFaceMap.end(); ++itr)
	{
		Face& face = itr->second;
		for(int j = 0; j < 3; ++j)
		{
			int iV1 = face.mVerticesID[j];
			int iV2 = face.mVerticesID[(j+1)%3];
			int iNeighbor = GetNeighborFaceByEdge(face.mID,Edge(iV1,iV2));
			face.mNeighborFaceID.push_back(iNeighbor);
		}
	}

	m_bFaceAdjacency = true;
}

void iCathMesh::GenVertexAdjacency()
{
	if(m_bVertexAdjacency)
		return;

	// Clear Stuff.
	for(VertexMap::iterator itr = mVertexMap.begin(); itr!=mVertexMap.end(); ++itr)
	{
		itr->second.mNeighborVertexID.clear();
		itr->second.mNeighborFaceID.clear();
	}
	
	// Find Every Vertex's Neighbor Face
	for(FaceMap::iterator itr = mFaceMap.begin(); itr!=mFaceMap.end(); ++itr)
	{
		Face& face = itr->second;
		for(int j = 0; j < 3; ++j)
			mVertexMap[face.mVerticesID[j]].mNeighborFaceID.push_back(itr->first);
	}

	// Create vertex neighbor for vertex;
	// !! Note that NeighborVertexID has duplication. There's a lot of room to optimize!	
	for(VertexMap::iterator itr = mVertexMap.begin(); itr!=mVertexMap.end(); ++itr)
	{
		Vertex& rVertex = itr->second;
		int faceNeighborCnt = rVertex.mNeighborFaceID.size();
		for(int j = 0; j < faceNeighborCnt; ++j)
		{
			const Face& rFace = mFaceMap[rVertex.mNeighborFaceID[j]];
			for(int k = 0; k < 3; ++k)
			{
				const Vertex& rV= mVertexMap[rFace.mVerticesID[k]];
				rVertex.mNeighborVertexID.push_back(rV.mID);
			}
		}
	}

	m_bVertexAdjacency = true;
}

void iCathMesh::InValidAdjacencyInfo()
{
	m_bEdgeAdjacency = false;
	m_bFaceAdjacency = false;
	m_bVertexAdjacency = false;
}

std::set<int> iCathMesh::FindAllConnectFaces(int seedId)
{
	if(seedId == -1)
		return std::set<int>();

	std::set<int> indices;
	std::vector<int> pingpong[2];
	indices.insert(seedId);
	pingpong[0].push_back(seedId);
	
	int i = 0;
	bool added = true;
	while(added)
	{
		added = false;
		while(!pingpong[i].empty())
		{
			int id = pingpong[i].back();
			const Face& f = mFaceMap[id];
			for(int j = 0; j < f.mNeighborFaceID.size(); j++)
			{
				int xid = f.mNeighborFaceID[j];
				if(xid != -1 && indices.find(xid) == indices.end())
				{
					indices.insert(xid);
					added = true;
					pingpong[(i+1)%2].push_back(xid);
				}
			}
			pingpong[i].pop_back();
		}
		i = (i+1)%2;
	}

	return indices;
}

void iCathMesh::FillHole()
{
	std::vector<int> vVertex;
	bool bOneHole = IsOnlyOneHole(&vVertex);
	
	if(!bOneHole)
	{
		printf("Failed to fill hole!............There're more than one hole!\n");
		return;
	}

	iv::vec3 vCenter(0.0f);
	int iVertexCount = vVertex.size();
	for(int i = 0; i < iVertexCount; ++i)
	{
		int iID = vVertex[i];
		vCenter += mVertexMap[iID].mPositions;
	}
	vCenter /= static_cast<float>(iVertexCount);

	GenEdgeAdjacency();

	Edge e(vVertex[0],vVertex[1]);
	int iFaceID = mEdges[e][0];
	const Face& rf = mFaceMap[iFaceID];
	
	int iV1,iV2;
	for(int i = 0; i < 3; ++i)
	{
		int i1 = rf.mVerticesID[i];
		int i2 = rf.mVerticesID[(i+1)%3];

		if(e == Edge(i1,i2))
		{
			iV1 = i2;
			iV2 = i1;
		}
	}

	iv::vec3 v1 = mVertexMap[iV1].mPositions;
	iv::vec3 v2 = mVertexMap[iV2].mPositions;

	iv::vec3 refNormal = iv::normalize(iv::cross(vCenter - v2,v1 - v2));

	Vertex nv;
	nv.mID = GenNextNewVertexID();
	nv.mPositions = vCenter;
	nv.mNormals = refNormal;
	mVertexMap[nv.mID] = nv;

	for(int i = 0; i < iVertexCount; ++i)
	{
		int iV1 = vVertex[i];
		int iV2 = vVertex[(i+1)%iVertexCount];

		Face nf;
		nf.mID = GetNextNewFaceID();
		nf.mVerticesID.push_back(iV1);
		nf.mVerticesID.push_back(iV2);
		nf.mVerticesID.push_back(nv.mID);
		mFaceMap[nf.mID] = nf;
		CorrectWinding(nf.mID,refNormal);
	}

	InValidAdjacencyInfo();
	GenAdjacency();	
}

int iCathMesh::GetNextNewFaceID()
{
	if(mFaceMap.size() == 0)
		return 0;
	FaceMap::reverse_iterator itr = mFaceMap.rbegin();
	return itr->first + 1;
}

bool iCathMesh::IsFaceInPositiveSide( int iFaceID,const iv::vec3& vPoint,const iv::vec3& vPositive )
{
	const iCathMesh::Face& f = mFaceMap[iFaceID];
	const iv::vec3& v1 = mVertexMap[f.mVerticesID[0]].mPositions;
	const iv::vec3& v2 = mVertexMap[f.mVerticesID[1]].mPositions;
	const iv::vec3& v3 = mVertexMap[f.mVerticesID[2]].mPositions;

	float f1 = iv::dot(v1-vPoint,vPositive);
	float f2 = iv::dot(v2-vPoint,vPositive);
	float f3 = iv::dot(v3-vPoint,vPositive);

	if( (f1 > 0.0f) && (f2 > 0.0f) && (f3 > 0.0f) )
		return true;
	else
		return false;
}

bool iCathMesh::FindFirstCircle( int iStart,const iv::vec3& vPoint,const iv::vec3& vDir,
	std::vector<int>& o_vFirstCircle )
{
	int iNeighborBuffer = -1;
	int iLastFace = -1;

	o_vFirstCircle.push_back(iStart);

	const Face &rf = mFaceMap[iStart];
	for (int i = 0; i < rf.mNeighborFaceID.size(); ++i)
	{
		int iNeighbor = rf.mNeighborFaceID[i];
		if (IsFaceCrossPlane(iNeighbor, vPoint, vDir))
		{
			iNeighborBuffer = iNeighbor;
			o_vFirstCircle.push_back(iNeighborBuffer);
			break;
		}
	}
	

	iLastFace = iStart;
	int count = 0;

	while (iNeighborBuffer != o_vFirstCircle[0])
	{
		count++;
		const Face& f = mFaceMap[iNeighborBuffer];

		bool found = false;
		int iCntNeighbor = f.mNeighborFaceID.size();

		if (iCntNeighbor != 3)
		{
			MessageBox(NULL, "Adjacency Info Error!", "Error!", MB_OKCANCEL);
			return false;
		}

		for (int i = 0; i < iCntNeighbor; ++i)
		{
			int iNeighbor = f.mNeighborFaceID[i];
			if ((iNeighbor != iLastFace) && IsFaceCrossPlane(iNeighbor, vPoint, vDir))
			{
				iLastFace = iNeighborBuffer;
				iNeighborBuffer = iNeighbor;
				o_vFirstCircle.push_back(iNeighborBuffer);
				found = true;
				break;
			}
		}

		if (!found)
		{
			MessageBox(NULL, "Adjacency Info Error!", "Error!", MB_OKCANCEL);
			return false;
		}

		// Avoid infinite Loop!
		if (o_vFirstCircle.size() > 5000)
		{
			MessageBox(NULL, "Fall into Infinite Loop!", "Error!", MB_OKCANCEL);
			return false;
		}
		if (count > 500)
		{
			return false;
		}
	}

	std::set<int> hell;
	for (int i = 0; i < o_vFirstCircle.size(); ++i)
		hell.insert(o_vFirstCircle[i]);
	if (hell.size() != o_vFirstCircle.size())
		o_vFirstCircle.pop_back();
	return true;
}

void iCathMesh::FindSecondCircle( const std::vector<int>& vFirstCircle,
	bool* pbCheck,const iv::vec3& vPoint,const iv::vec3& vDir,std::vector<int>& o_vSecondCircle )
{
	int iFaceCnt = vFirstCircle.size();
	for (int i = 0; i < iFaceCnt; ++i)
	{
		int id = vFirstCircle[i];
		pbCheck[id] = true;
		const iCathMesh::Face& rf = mFaceMap[id];
		int neighborCnt = rf.mNeighborFaceID.size();
		for (size_t j = 0; j < neighborCnt; ++j)
		{
			int iNeighbor = rf.mNeighborFaceID[j];
			if (!pbCheck[iNeighbor] && IsFaceInPositiveSide(iNeighbor, vPoint, vDir))
			{
				o_vSecondCircle.push_back(iNeighbor);
				pbCheck[iNeighbor] = true;
			}
		}
	}
}

void iCathMesh::GetNarrowedFaces(int iStart,int iEnd,std::vector<std::vector<int> >& o_vNarrowedFace)
{
	iv::vec3 v11, v12, v13;
	GetFaceVerticesByIndex(iStart, v11, v12, v13);
	iv::vec3 v1 = (v11 + v12 + v13) / 3.0f;

	iv::vec3 v21, v22, v23;
	GetFaceVerticesByIndex(iEnd, v21, v22, v23);
	iv::vec3 v2 = (v21 + v22 + v23) / 3.0f;

	iv::vec3 vRefDir = iv::normalize(v2 - v1);

	std::vector<int> vFirstRound;
	FindFirstCircle(iStart, v1, vRefDir, vFirstRound);

	// Add First Circle
	o_vNarrowedFace.push_back(vFirstRound);

	float dis = iv::length(mFaceMap[iStart].mGravity - mFaceMap[iEnd].mGravity);
	if (dis <= 0.008)
	{
		return;
	}

	bool *bChecked = new bool[mFaceMap.size()];
	memset(bChecked, 0, mFaceMap.size() * sizeof(bool));
	std::vector<int> vPing, vPong;
	FindSecondCircle(vFirstRound, bChecked, v1, vRefDir, vPing);

	// Add Second Circle. 
	o_vNarrowedFace.push_back(vPing);

	int iFaceCnt = vFirstRound.size();

	vFirstRound.insert(vFirstRound.end(), vPing.begin(), vPing.end());

	int iSwapFlag = 1;
	int iTime = 0;
	bool bNeed2Break = false;

	while ((vFirstRound.size() - iFaceCnt) != 0)
	{
		if (bNeed2Break)
			break;
		iFaceCnt = vFirstRound.size();
		switch (iSwapFlag)
		{
		case 1:
		{
			int iCnt1 = vPing.size();
			for (int i = 0; i < iCnt1; ++i)
			{
				const Face& rf = mFaceMap[vPing[i]];
				int iNeighborCnt = rf.mNeighborFaceID.size();
				for (int j = 0; j < iNeighborCnt; ++j)
				{
					int iNeighbor = rf.mNeighborFaceID[j];
					if (!bChecked[iNeighbor])
					{
						if (iNeighbor == iEnd)
							bNeed2Break = true;
						vPong.push_back(iNeighbor);
						bChecked[iNeighbor] = true;
					}
				}
			}
			vFirstRound.insert(vFirstRound.end(), vPong.begin(), vPong.end());
			o_vNarrowedFace.push_back(vPong);
			vPing.clear();
			iSwapFlag = 2;
			++iTime;
		}
		break;
		case 2:
		{
			int iCnt2 = vPong.size();
			for (int i = 0; i < iCnt2; ++i)
			{
				const Face& rf = mFaceMap[vPong[i]];
				int iNeighborCnt = rf.mNeighborFaceID.size();
				for (int j = 0; j < iNeighborCnt; ++j)
				{
					int iNeighbor = rf.mNeighborFaceID[j];
					if (!bChecked[iNeighbor])
					{
						if (iNeighbor == iEnd)
							bNeed2Break = true;
						vPing.push_back(iNeighbor);
						bChecked[iNeighbor] = true;
					}
				}
			}
			vFirstRound.insert(vFirstRound.end(), vPing.begin(), vPing.end());
			o_vNarrowedFace.push_back(vPing);
			vPong.clear();
			iSwapFlag = 1;
			++iTime;
		}
		break;
		default:
			break;
		}
	}

	iFaceCnt = vFirstRound.size();

	printf("Narrowing part face cnt : %d. %d Circles\n", iFaceCnt, o_vNarrowedFace.size());
	std::set<int> exclusive;
	for (int i = 0; i < iFaceCnt; ++i)
		exclusive.insert(vFirstRound[i]);

	if (iFaceCnt == exclusive.size())
		printf("All Exclusive\n");
	else
		printf("Not Exclusive! %d If No Duplication!\n", exclusive.size());

	delete[] bChecked;
}

void iCathMesh::MoveFaceToCenter( const std::vector<std::vector<int> >& vFace )
{
	// Calculate Central Line.
	std::vector<iv::vec3> vMid;

	int iCircleCount = vFace.size();

	for(int i = 0; i < iCircleCount; ++i)
	{
		iv::vec3 c(0.0f);
		for(int j = 0; j < vFace[i].size(); ++j)
		{
			const Face& rf = mFaceMap[vFace[i][j]];
			for(int k = 0; k < 3; ++k)
				c += mVertexMap[rf.mVerticesID[k]].mPositions;
		}
		c /= static_cast<float>(vFace[i].size()*3);
		vMid.push_back(c);
	}

	NarrowMap narrMap;

	for(int i = 0; i < iCircleCount; ++i)
	{
		const iv::vec3& vCenter = vMid[i];
		int iCircleFaceCount = vFace[i].size();
		for(int j = 0; j < iCircleFaceCount; ++j)
		{
			const Face& rf = mFaceMap[vFace[i][j]];
			for(int k = 0; k < 3; ++k)
			{
				int iVertexID = rf.mVerticesID[k];
				
				if(narrMap.find(iVertexID)!=narrMap.end())
					continue;

				iv::vec3& rV = mVertexMap[iVertexID].mPositions;
				narrMap[iVertexID] = rV;
				rV += (vCenter - rV) * 0.25f;
			}
		}
	}

	mNarrows.push_back(narrMap);

	mbDirty = true;
}

void iCathMesh::GetNarrowPoints( std::vector<std::shared_ptr<CxPoints> >& vNarrowPoints )
{
	vNarrowPoints.clear();
	for(int i = 0; i < mNarrows.size(); ++i)
	{
		std::shared_ptr<CxPoints> points = std::shared_ptr<CxPoints>(new CxPoints);
		NarrowMap::iterator itr;
		for(itr = mNarrows[i].begin(); itr != mNarrows[i].end(); ++itr)
		{
			points->AddPoint(itr->second);
		}
		vNarrowPoints.push_back(points);
	}
}

BoundingBox iCathMesh::getBoundingBox()
{

#undef min
#undef max

	float xMax = std::numeric_limits<float>::min();
	float yMax = xMax;
	float zMax = xMax;

	float xMin = std::numeric_limits<float>::max();
	float yMin = xMin;
	float zMin = xMin;

	for(auto itr = mVertexMap.begin(); itr != mVertexMap.end(); ++itr)
	{
		const iv::vec3& p = itr->second.mPositions;

		xMax = std::max(p.x,xMax);
		xMin = std::min(p.x,xMin);

		yMax = std::max(p.y,yMax);
		yMin = std::min(p.y,yMin);

		zMax = std::max(p.z,zMax);
		zMin = std::min(p.z,zMin);
	}

	BoundingBox bb;
	bb.vCenter.x = (xMax + xMin) * 0.5f;
	bb.vCenter.y = (yMax + yMin) * 0.5f;
	bb.vCenter.z = (zMax + zMin) * 0.5f;

	bb.vExtent.x = (xMax - xMin) * 0.5f;
	bb.vExtent.y = (yMax - yMin) * 0.5f;
	bb.vExtent.z = (zMax - zMin) * 0.5f;

	return bb;
}

void iCathMesh::_createEdgeLines()
{
	if(!m_bEdgeAdjacency)
		GenEdgeAdjacency();

	if(m_EdgeLines)
		m_EdgeLines.reset();

	std::set<Edge> edgeSet;
	 
	m_EdgeLines = std::make_shared<CxLines>();

	for(auto itr = mEdges.begin(); itr != mEdges.end(); ++itr)
	{
		if(itr->second.size() != 2)
		{
			const Edge& edge = itr->first;
			edgeSet.insert(edge);
			const Vertex& v1 = mVertexMap[edge.iV1];
			const Vertex& v2 = mVertexMap[edge.iV2];
			m_EdgeLines->AddLines(v1.mPositions,v2.mPositions);
 		}
	}
	_sortHoleCircles(edgeSet);

	m_DuplicateMap.clear();
}

void iCathMesh::_sortHoleCircles(std::set<Edge>& edgePool)
{
	if(edgePool.empty())
		return;
	
	std::vector<Edge> hole;

	// setup
	auto itr = edgePool.begin();
	int start = itr->iV1;
	int last = start;
	int next = itr->iV2;
	hole.push_back(*itr);
	edgePool.erase(itr);
	m_HoleEdges.clear();

	while(!edgePool.empty())
	{
		for(auto itr = edgePool.begin(); itr != edgePool.end();)
		{
			if(itr->containVertex(next))
			{
				last = next;
				next = itr->iV1 == last ?  itr->iV2 : itr->iV1;
				hole.push_back(*itr);
				edgePool.erase(itr);
				if(next == start)
					break;
				else
					itr = edgePool.begin();
			}
			else
				++itr;
		}

		m_HoleEdges.push_back(hole);
		hole.clear();

		//setup for next hole
		if(edgePool.empty())
			break;
		auto tmp = edgePool.begin();
		start = tmp->iV1;
		last = start;
		next = tmp->iV2;
		hole.push_back(*tmp);
		edgePool.erase(tmp);
	}
}

void iCathMesh::RefineVertexPosition()
{
	std::map<int,iv::vec3> ps;
	std::ifstream mem("abc.txt");
	while(!mem.eof())
	{
		int id;
		float x,y,z;
		mem >> id >> x >> y >> z;
		ps.insert(std::make_pair(id,iv::vec3(x,y,z)));//22222222222222222222
	}

	for(auto itr = ps.begin(); itr != ps.end(); ++itr)
	{
		mVertexMap[itr->first].mPositions = itr->second;
	}
}


void iCathMesh::setActiveEdgeIndex(int index)
{
	if(index == m_iActiveEdgeIndex)
		return;
	if(index >= m_HoleEdges.size())
		m_iActiveEdgeIndex = m_HoleEdges.size()-1;
	else if(index < 0)
		m_iActiveEdgeIndex = 0;
	else
		m_iActiveEdgeIndex = index;

	printf("Current active hole : %d.\n",m_iActiveEdgeIndex);

	if(!m_ActiveEdgeLines)
		m_ActiveEdgeLines = std::make_shared<CxLines>();

	m_ActiveEdgeLines->Clear();

	const std::vector<Edge>& holeEdges = m_HoleEdges[m_iActiveEdgeIndex];

	std::set<int> edgeVertices;

	std::map<int,iv::vec3> shitMap;
	std::ofstream shitLog("abc.txt",std::ios::ate);

	for(int i = 0; i < holeEdges.size(); ++i)
	{
		const Edge& edge = holeEdges[i];

		edgeVertices.insert(edge.iV1);
		edgeVertices.insert(edge.iV2);

		std::vector<int>& sb = mEdges[edge];
		const iv::vec3& p1 = mVertexMap[edge.iV1].mPositions;
		const iv::vec3& p2 = mVertexMap[edge.iV2].mPositions;

		if(shitMap.find(edge.iV1) == shitMap.end())
			shitMap.insert(std::make_pair(edge.iV1,p1));
		if(shitMap.find(edge.iV2) == shitMap.end())
			shitMap.insert(std::make_pair(edge.iV2,p2));

		printf("edge <%d,%d> : (%f,%f,%f)-(%f,%f,%f)\n",edge.iV1,edge.iV2,
			p1.x,p1.y,p1.z,
			p2.x,p2.y,p2.z);

		for(int j = 0; j < sb.size(); ++j)
		{
			const Face& face = mFaceMap[sb[j]];
			m_ActiveEdgeLines->AddLines(mVertexMap[face.mVerticesID[0]].mPositions,
				mVertexMap[face.mVerticesID[1]].mPositions);		
			m_ActiveEdgeLines->AddLines(mVertexMap[face.mVerticesID[1]].mPositions,
				mVertexMap[face.mVerticesID[2]].mPositions);	
			m_ActiveEdgeLines->AddLines(mVertexMap[face.mVerticesID[2]].mPositions,
				mVertexMap[face.mVerticesID[0]].mPositions);	
		}
	}
	for(auto itr = shitMap.begin(); itr != shitMap.end();++itr)
		shitLog << itr->first << " " << itr->second.x << " " << itr->second.y << 
		" " << itr->second.z << std::endl;

	shitLog.close();

	printf("Before remove duplications: %d vertices.",edgeVertices.size());
	std::vector<int> tmp;
	m_DuplicateMap.clear();
	for(auto itr = edgeVertices.begin(); itr != edgeVertices.end(); ++itr)
		tmp.push_back(*itr);
	for(int i = 0; i < tmp.size(); ++i)
	{
		iv::vec3 pThis = mVertexMap[tmp[i]].mPositions;
		for(int j = i + 1; j < tmp.size(); ++j)
		{
			if(tmp[i] == tmp[j])
				continue;
			iv::vec3 pThat = mVertexMap[tmp[j]].mPositions;
			float len = iv::length(pThat - pThis);
			if(len < 0.01)
			{
				m_DuplicateMap[tmp[j]] = tmp[i];
				tmp[j] = tmp[i];
			}
		}
	}
	edgeVertices.clear();
	for(int i = 0; i < tmp.size(); ++i)
	{
		edgeVertices.insert(tmp[i]);
	}

	printf("After remove duplication: %d.\n",edgeVertices.size());
	for(auto itr = m_DuplicateMap.begin(); itr != m_DuplicateMap.end(); ++itr)
	{
		printf("(%d-->%d)\n",itr->first,itr->second);
	}
}

void iCathMesh::fillActiveHole()
{
	if(m_iActiveEdgeIndex >= m_HoleEdges.size() || m_iActiveEdgeIndex < 0)
		return;

	const std::vector<Edge>& holeEdges = m_HoleEdges[m_iActiveEdgeIndex];

	//计算中心点
	iv::vec3 vCenter(0.0f);
	for(int i = 0; i < holeEdges.size(); ++i)
	{
		vCenter += mVertexMap[holeEdges[i].iV1].mPositions;
		vCenter += mVertexMap[holeEdges[i].iV2].mPositions;
	}
	float rc = 1.0f / (2.0f * static_cast<float>(holeEdges.size()));
	vCenter = vCenter * rc;


	const Face& face = mFaceMap[mEdges[holeEdges[0]][0]];

	int iV1,iV2;

	for(int i = 0; i < 3; ++i)
	{
		int i1 = face.mVerticesID[i];
		int i2 = face.mVerticesID[(i+1)%3];
		if(Edge(i1,i2) == holeEdges[0])
		{
			iV1 = i2;
			iV2 = i1;
		}
	}

	const iv::vec3& v1 = mVertexMap[iV1].mPositions;
	const iv::vec3& v2 = mVertexMap[iV2].mPositions;

	iv::vec3 refNorm = iv::normalize(iv::cross(v1 - vCenter,v2 - vCenter));

	auto itr = mVertexMap.rbegin();
	Vertex v;
	v.mID = itr->second.mID + 1;
	v.mPositions = vCenter;
	v.mNormals = refNorm;
	mVertexMap[v.mID] = v;

	for(int i = 0; i < holeEdges.size(); ++i)
	{
		Face nf;
		nf.mID = mFaceMap.rbegin()->second.mID + 1;
		nf.mVerticesID.push_back(holeEdges[i].iV1);
		nf.mVerticesID.push_back(holeEdges[i].iV2);
		nf.mVerticesID.push_back(v.mID);
		mFaceMap[nf.mID] = nf;
		CorrectWinding(nf.mID,refNorm);
	}

	InValidAdjacencyInfo();
	GenAdjacency();	

	SetDirty();

	_createEdgeLines();
}

void iCathMesh::removeDuplicates()
{

	for(auto itr = m_DuplicateMap.begin(); itr != m_DuplicateMap.end(); ++itr)
	{
		for(auto fitr = mFaceMap.begin();
			fitr != mFaceMap.end();
			++fitr)
		{
			Face& face = fitr->second;
			for(int i = 0; i < 3; ++i)
			{
				if(face.mVerticesID[i] == itr->first)
					face.mVerticesID[i] = itr->second;
			}
		}
	}

	InValidAdjacencyInfo();
	GenAdjacency();
	SetDirty();
	_createEdgeLines();
}

void iCathMesh::removeThisEdgeAndNeighborFaces()
{
	const std::vector<Edge>& holeEdges = m_HoleEdges[m_iActiveEdgeIndex];

	for(int i = 0; i < holeEdges.size(); ++i)
	{
		const Edge& edge = holeEdges[i];

		std::vector<int>& sb = mEdges[edge];

		for(int j = 0; j < sb.size(); ++j)
		{
			auto itr = mFaceMap.find(sb[j]);
			if(itr != mFaceMap.end())
				mFaceMap.erase(itr);
		}
	}

	InValidAdjacencyInfo();
	GenAdjacency();
	SetDirty();
	_createEdgeLines();
}

void iCathMesh::reverseSelectedFace()
{
	if(m_iSelectedFaceIndex < 0)
		return;
	_reverseFaceWinding(m_iSelectedFaceIndex);
}

void iCathMesh::_reverseFaceWinding(int index)
{
	if(index < 0)
		return;
	Face& face = mFaceMap[index];
	int tmp = face.mVerticesID[1];
	face.mVerticesID[1] = face.mVerticesID[2];
	face.mVerticesID[2] = tmp;

	SetDirty();
}

std::shared_ptr<iCathMesh> iCathMesh::CreateSubMeshBySelectFace(int faceId)
{
	// 1834
	//m_iSelectedFaceIndex = 3480;

#if 0
	// 
	std::set<int> indices = FindAllConnectFaces(5165);
	std::vector<int> tmp;
	for(auto itr = indices.begin();
		itr != indices.end(); ++itr)
		tmp.push_back(*itr);
	return CreateSubMesh(tmp);
#endif


#if 1
	std::set<int> part1 = FindAllConnectFaces(5165);
	std::vector<int> tmp;
	for(auto itr = mFaceMap.begin(); itr != mFaceMap.end(); ++itr)
	{
		int id = itr->first;
		if(part1.find(id) == part1.end())
			tmp.push_back(id);
	}

	return CreateSubMesh(tmp);

#endif
}



void iCathMesh::CalcPerVertexNormal()
{
	for(auto itr = mVertexMap.begin(); itr != mVertexMap.end(); ++itr)
	{
		Vertex& rVertex = itr->second;

		iv::vec3 vNorm(0.0f);
		int iNeighbor = rVertex.mNeighborFaceID.size();
		
		for(int j = 0; j < iNeighbor; ++j)
			vNorm += mFaceMap[rVertex.mNeighborFaceID[j]].mNormal;

		vNorm /= static_cast<float>(iNeighbor);
		vNorm = iv::normalize(vNorm);
		rVertex.mNormals = vNorm;
	}
}




void iCathMesh::removeSelectedFace()
{

	if(m_iSelectedFaceIndex < 0)
		return;
	auto itr = mFaceMap.find(m_iSelectedFaceIndex);
	if(itr != mFaceMap.end())
		mFaceMap.erase(itr);

	InValidAdjacencyInfo();
	GenAdjacency();
	SetDirty();
	_createEdgeLines();
}

void iCathMesh::saveAsObj(const char* pStrFilePath)
{
	std::vector<Face> vFace;
	std::vector<Vertex> vVertex;
	MakeThingsRight(vFace,vVertex);

	std::ofstream oFile(pStrFilePath);

	for(auto itr = mVertexMap.begin(); itr != mVertexMap.end(); ++itr)
	{
		const iv::vec3& p = itr->second.mPositions;
		const iv::vec3& n = itr->second.mNormals;
		oFile << "vn " << n.x << " " << n.y << " " << n.z << std::endl;
		oFile << "v " << p.x << " " << p.y << " " << p.z << std::endl;
	}

	for(auto itr = mFaceMap.begin(); itr != mFaceMap.end(); ++itr)
	{
		const std::vector<int>& indices = itr->second.mVerticesID;
		oFile << "f " << indices[0] + 1 << "//" << indices[0] + 1 << " " <<
			indices[1] + 1 << "//" << indices[1] + 1 << " " << indices[2] + 1 << 
			"//" << indices[2]+1 << std::endl;
	}
	oFile.close();

	printf("Save Succeeded!\n");
}

void iCathMesh::reverseFacesAndSaveAsObj()
{
	for(auto itr = mFaceMap.begin(); itr != mFaceMap.end(); ++itr)
	{
		_reverseFaceWinding(itr->first);
	}
	saveAsObj("fuck.obj");
}


void iCathMesh::SelectFaceForDeform_TumorMode(const iv::vec3& p)
{	
	
	clearSelectedFace();

	int q = findCrossFaceId_InTumorFace(p);
	if (q == -1)
	{
		std::cout << "Triangles are not in the optional range" << std::endl;
		return;
	}
	
	Face f1 = mFaceMap[q];
	face_ids_control.push_back(q);
	IntArray a;
	a.push_back(q);
	for (int i = 0 ; i < f1.mNeighborFaceID.size();i++)
	{
		a.push_back(f1.mNeighborFaceID[i]);
	}	

	getMoreFaceInTumorFace(q,mVesselRadius*0.5,face_ids_free);

}

void iCathMesh::getMoreFaceInTumorFace(int face_id, float thickness,IntArray& result)
{
	Face f1 = mFaceMap[face_id];
	Face f2 = mFaceMap[f1.mNeighborFaceID[0]];
	iv::vec3 n = f1.mNormal;
	
	iv::vec3 p = f1.mGravity;
	float d = -(n.x*p.x + n.y*p.y + n.z*p.z);
	
	//float thickness = mVesselRadius*0.5;

	for (int i = 0; i < m_TumorFace.size(); i++)
	{
		Face& rFace = mFaceMap[m_TumorFace[i]];
		float e = n.x * rFace.mGravity.x + n.y * rFace.mGravity.y + n.z * rFace.mGravity.z;
		
		if (((e + d) >= -thickness) && ((e + d) <= thickness))
		{
			result.push_back(rFace.mID);
		}
	}
	
}
void iCathMesh::getMoreFace(const iv::vec3 &p,float thickness ,IntArray &vCollideFace)
{
	//float thickness = mVesselRadius*1.5f;

	for (int i = 0; i < mFaceMapInit.size(); i++)
	{
		if (mFaceMap.find(i) != mFaceMap.end())
		{
			Face& rFace = mFaceMapInit[i];
			iv::vec3 g = rFace.mGravity;

			if ((g.x >= p.x - thickness) && (g.x <= p.x + thickness))
			{
				if ((g.y >= p.y - thickness) && (g.y <= p.y + thickness))
				{
					if ((g.z >= p.z - thickness) && (g.z <= p.z + thickness))
					{
						vCollideFace.push_back(rFace.mID);
					}
				}
			}
		}
		else
		{
			continue;
		}
		
	}
}




void iCathMesh::doArapmodelingPrecompute()
{	
	// Record backup mesh
	//mFaceMapBackup.push_back(mFaceMap) ;
	//mVertexMapBackup.push_back(mVertexMap) ;	
	
	
	// 得到所有所有相关点，只需要考虑这些点而不需要考虑网格中全部的点
	getVIDByFID(face_ids_all, vertex_ids_all);
	
	// 得到应用于Solver的顶点和面的信息
	Eigen::MatrixXd vertices;
	Eigen::MatrixXi faces;	
	getVexticesMatrix(vertices, vertex_ids_all);	
	getFaceMatrix(faces, face_ids_all);	
	
	Eigen::VectorXi fixed_vertices_index;	
	getFixedVerticesIdVextor(vertex_ids_all, face_ids_free, face_ids_control, fixed_vertices_index);
	getIndexes(fixed_vertices_index, face_ids_control,index_in_fixed_vertices_id);		
	getVerticesCoordinateById(fixed_vertices_index, fixed_vertices_coordinate);
	
	// Solver初始化
	solver = new arap::demo::ArapSolver(vertices, faces, fixed_vertices_index, 4);

	// 预计算，一次变形只需要进行一次预计算
	solver->Precompute();
	
}

void iCathMesh::doArapmodelingPrecompute_TumorMode( )
{
	if ((face_ids_control.size() == 0) || (face_ids_free.size() == 0))
		return;

	std::cout << "#doArapmodelingPrecompute_TumorMode"  << std::endl;
	/*// Record backup mesh
	mFaceMapBackup.push_back(mFaceMap);
	mVertexMapBackup.push_back(mVertexMap);*/

	// 得到所有所有相关点，只需要考虑这些点而不需要考虑网格中全部的点
	face_ids_all = m_TumorFace;
	getVIDByFID(face_ids_all, vertex_ids_all);

	// 得到应用于Solver的顶点和面的信息
	Eigen::MatrixXd vertices;
	Eigen::MatrixXi faces;	
	getVexticesMatrix(vertices, vertex_ids_all);	
	getFaceMatrix(faces, face_ids_all);

		
	Eigen::VectorXi fixed_vertices_index;
	getFixedVerticesIdVextor(vertex_ids_all, face_ids_free, face_ids_control, fixed_vertices_index);
	getIndexes(fixed_vertices_index, face_ids_control, index_in_fixed_vertices_id);
	getVerticesCoordinateById(fixed_vertices_index, fixed_vertices_coordinate);

	// Solver初始化
	solver = new arap::demo::ArapSolver(vertices, faces, fixed_vertices_index, 4);
	
	solver->Precompute();

}


void iCathMesh::doDeform(const iv::vec3& offset,int flag)
{		
	if ((face_ids_control.size() == 0) || (face_ids_free.size() == 0))
		return;

	std::cout << "doDeform" << std::endl;
	// flag = 0,是构造血管狭窄，需要取消之前的高亮显示
	if (flag == 0)
	{
		m_SelectedFace->Clear();	
	}

	// 设置'控制点'的固定位置约束，并近似ARAP运算
	setContrlVerticesCoordinate(face_ids_control, index_in_fixed_vertices_id, offset, fixed_vertices_coordinate, flag);	
	solver->Solve(fixed_vertices_coordinate);	
	Eigen::MatrixXd update_vertices = solver->GetVertexSolution();
	
	//更新点的位置
	updateVerticesMap(update_vertices, vertex_ids_all);	

	if (flag == 1) // 肿瘤
	{
		CalcPerTumorFaceNormal();
		CalcPerTumorVertexNormal();
	}
	if (flag == 0) // 狭窄
	{
		CalcPerFaceNormal(face_ids_all);
		CalcPerVertexNormal(face_ids_all);
	}	
	SetDirty();			
}

void iCathMesh::CalcPerFaceNormal(IntArray& faces_id)
{
	printf("Calculating faceID Face Normal........................");

	for (int i = 0; i < faces_id.size(); i++)
	{
		Face& rFace = mFaceMap[faces_id[i]];
		iv::vec3 v1, v2, v3;
		//Get Face Vertices By Index
		v1 = mVertexMap[rFace.mVerticesID[0]].mPositions;
		v2 = mVertexMap[rFace.mVerticesID[1]].mPositions;
		v3 = mVertexMap[rFace.mVerticesID[2]].mPositions;
		rFace.mNormal = iv::normalize(iv::cross(v3 - v2, v1 - v2));
	}

	printf("Done.\n");
}

void iCathMesh::CalcPerTumorFaceNormal()
{
	printf("Calculating Face Normal........................");
	IntArray more_face ;
	getMoreNeighber(m_TumorFace, more_face);
	getMoreNeighber(more_face, more_face);
	getMoreNeighber(more_face, more_face);

	for (int i = 0; i < more_face.size(); i++)
	{	
		Face& rFace = mFaceMap[more_face[i]];
		iv::vec3 v1, v2, v3;		
		v1 = mVertexMap[mFaceMap[more_face[i]].mVerticesID[0]].mPositions;
		v2 = mVertexMap[mFaceMap[more_face[i]].mVerticesID[1]].mPositions;
		v3 = mVertexMap[mFaceMap[more_face[i]].mVerticesID[2]].mPositions;
		rFace.mNormal = iv::normalize(iv::cross(v3 - v2, v1 - v2));		
	}	
	printf("Done.\n");
}

void iCathMesh::CalcPerTumorFaceNormalForSim()
{	
	printf("Calculating Face Normal........................");
	IntArray m_TumorFace1;
	getMoreNeighber(m_TumorFace, m_TumorFace1);
	for (int i = 0; i < m_TumorFace1.size(); i++)
	{		
		Face& rFace = mFaceMap[m_TumorFace1[i]];
		iv::vec3 v1, v2, v3;
		//Get Face Vertices By Index
		v1 = mVertexMap[mFaceMap[m_TumorFace1[i]].mVerticesID[0]].mPositions;
		v2 = mVertexMap[mFaceMap[m_TumorFace1[i]].mVerticesID[1]].mPositions;
		v3 = mVertexMap[mFaceMap[m_TumorFace1[i]].mVerticesID[2]].mPositions;
		rFace.mNormal = iv::normalize(iv::cross(v3 - v2, v1 - v2));
	}	
	printf("Done.\n");
}

void iCathMesh::CalcPerVertexNormal(IntArray& faces_id)
{
	IntArray verID;
	getVIDByFID(faces_id, verID);

	// 顶点的法向为相邻面的法向求平均
	for (int i = 0; i < verID.size(); i++)
	{
		Vertex& rVertex = mVertexMap[verID[i]];
		iv::vec3 vNorm(0.5f);
		int iNeighbor = rVertex.mNeighborFaceID.size();

		for (int j = 0; j < iNeighbor; ++j)
			vNorm += mFaceMap[rVertex.mNeighborFaceID[j]].mNormal;

		vNorm /= static_cast<float>(iNeighbor);
		vNorm = iv::normalize(vNorm);
		rVertex.mNormals = vNorm;
	}
}

void iCathMesh::CalcPerTumorVertexNormal()
{
	m_TumorVertex.clear();
	IntArray more_face;
	
	getMoreNeighber(m_TumorFace, more_face);
	getMoreNeighber(more_face, more_face);
	getMoreNeighber(more_face, more_face);
	getVIDByFID(more_face, m_TumorVertex);

	//drawTriangles(more_face);
	
	for (int i = 0 ; i < m_TumorVertex.size();i++)
	{
		Vertex& rVertex = mVertexMap[m_TumorVertex[i]];
		iv::vec3 vNorm(0.5f);
		int iNeighbor = rVertex.mNeighborFaceID.size();	

		for (int j = 0; j < iNeighbor; ++j)
			vNorm += mFaceMap[rVertex.mNeighborFaceID[j]].mNormal;

		vNorm /= static_cast<float>(iNeighbor);
		vNorm = iv::normalize(vNorm);
		rVertex.mNormals = vNorm;
	}
}

void iCathMesh::CalcPerTumorVertexNormalForSim()
{
	m_TumorVertex.clear();
	
	IntArray m_TumorFace1;
	getMoreNeighber(m_TumorFace, m_TumorFace1);
	getVIDByFID(m_TumorFace1, m_TumorVertex);

	//drawTriangles(more_face);

	for (int i = 0; i < m_TumorVertex.size(); i++)
	{
		Vertex& rVertex = mVertexMap[m_TumorVertex[i]];
		iv::vec3 vNorm(0.5f);
		int iNeighbor = rVertex.mNeighborFaceID.size();	

		for (int j = 0; j < iNeighbor; ++j)
			vNorm += mFaceMap[rVertex.mNeighborFaceID[j]].mNormal;

		vNorm /= static_cast<float>(iNeighbor);
		vNorm = iv::normalize(vNorm);
		rVertex.mNormals = vNorm;
	}
}

void iCathMesh::getMoreNeighber(IntArray faces_id_now, IntArray& faces_id_more)
{
	for (int i = 0; i < faces_id_now.size(); i++)
	{
		IntArray iNeighber = mFaceMap[faces_id_now[i]].mNeighborFaceID;
		for (int j = 0; j < iNeighber.size(); j++)
		{
			faces_id_more.push_back(iNeighber[j]);
			IntArray jNeighber = mFaceMap[iNeighber[j]].mNeighborFaceID;
			for (int q = 0; q < jNeighber.size(); q++)
			{
				faces_id_more.push_back(jNeighber[q]);
			}
		}
	}

	std::sort(faces_id_more.begin(), faces_id_more.end());
	faces_id_more.erase(unique(faces_id_more.begin(), faces_id_more.end()), faces_id_more.end());
}

void iCathMesh::drawTriangles(IntArray faces_id)
{
	if (faces_id.empty())
		m_iSelectedFaceIndex = -1;
	else
		m_iSelectedFaceIndex = faces_id[0];

	if (!m_SelectedFace)
		m_SelectedFace = std::make_shared<CxLines>();

	m_SelectedFace->Clear();

	for (int j = 0; j < faces_id.size(); ++j)
	{
		const Face& face = mFaceMap[faces_id[j]];
		
		m_SelectedFace->AddLines(mVertexMap[face.mVerticesID[0]].mPositions,
			mVertexMap[face.mVerticesID[1]].mPositions);
		m_SelectedFace->AddLines(mVertexMap[face.mVerticesID[1]].mPositions,
			mVertexMap[face.mVerticesID[2]].mPositions);
		m_SelectedFace->AddLines(mVertexMap[face.mVerticesID[2]].mPositions,
			mVertexMap[face.mVerticesID[0]].mPositions);
	}
}



void iCathMesh::getVIDByFID(const IntArray faces_id, IntArray& vertices_id)
{
	vertices_id.clear();
	// 得到'faces_id'中所有的顶点
	for (int i = 0; i < faces_id.size(); i++)
	{
		Face &face = mFaceMap[faces_id[i]];
		vertices_id.emplace_back(face.mVerticesID[0]);
		vertices_id.emplace_back(face.mVerticesID[1]);
		vertices_id.emplace_back(face.mVerticesID[2]);
	}	
	// 排序去重
	sort(vertices_id.begin(), vertices_id.end());
	vertices_id.erase(unique(vertices_id.begin(), vertices_id.end()), vertices_id.end());
}



void iCathMesh::getVexticesMatrix(Eigen::MatrixXd& vertex_matrix, const IntArray vertices_id)
{
	vertex_matrix.resize(vertices_id.size(), 3);
	for (int i = 0; i < vertices_id.size(); i++)
	{
		vertex_matrix(i, 0) = mVertexMap[vertices_id[i]].mPositions.x;
		vertex_matrix(i, 1) = mVertexMap[vertices_id[i]].mPositions.y;
		vertex_matrix(i, 2) = mVertexMap[vertices_id[i]].mPositions.z;
	}
}

void iCathMesh::getFaceMatrix(Eigen::MatrixXi& face_matrix, const IntArray faces_id)
{
	face_matrix.resize(faces_id.size(), 3);
	for (int i = 0; i < faces_id.size(); i++)
	{
		for (int j = 0 ; j < 3 ; j++)
		{
			std::vector <int>::iterator iElement = std::find(vertex_ids_all.begin(),
				vertex_ids_all.end(), mFaceMap[faces_id[i]].mVerticesID[j]);
			if (iElement != vertex_ids_all.end())
			{
				int nPosition = std::distance(vertex_ids_all.begin(), iElement);
				face_matrix(i, j) = nPosition;
			}
		}		
	}
}



void iCathMesh::getFixedVerticesIdVextor(const IntArray vertex_ids_all,
	const IntArray face_ids_free,
	const IntArray face_ids_control,
	Eigen::VectorXi& fixed_vertices_index)
{
	std::vector<int> free_vertices_ids;	
	getVIDByFID(face_ids_free, free_vertices_ids);	

	// 将'控制点'从'自由点'中删除
	for (int j = 0 ; j < face_ids_control.size(); j++)
	{
		for (int i = 0; i < 3; i++)
		{
			int VID = mFaceMap[face_ids_control[j]].mVerticesID[i];			
			std::vector<int>::iterator iter = std::find(free_vertices_ids.begin(),
				free_vertices_ids.end(), VID);
			if (iter != free_vertices_ids.end())
				free_vertices_ids.erase(iter);
		}		
	}	

	// 从'所有相关点'中删除'自由点'后，剩余均为固定点
	// 固定点包括：（1）控制点（2）处在外围，不在变形影响范围内的点
	fixed_vertices_index.resize(vertex_ids_all.size() - free_vertices_ids.size());
	int count_fixd = 0;
	for (int i = 0; i < vertex_ids_all.size(); i++)
	{
		Vertex &ver = mVertexMap[vertex_ids_all[i]];
		int nRet = std::count(free_vertices_ids.begin(), free_vertices_ids.end(), ver.mID);
		if (nRet == 0)	// 如果该点不是自由点，则将其加入固定点集合
			fixed_vertices_index(count_fixd++) = i;			
	}	

}

void iCathMesh::getVerticesCoordinateById(const Eigen::VectorXi fixed_vertices_index,
	Eigen::MatrixXd& fixed_vertices_coordinate)
{
	fixed_vertices_coordinate.resize(fixed_vertices_index.size(), 3);
	for (size_t i = 0; i < fixed_vertices_index.size(); i++)
	{
		Vertex v1 = mVertexMap[vertex_ids_all[fixed_vertices_index[i]]];
		fixed_vertices_coordinate(i, 0) = v1.mPositions.x;
		fixed_vertices_coordinate(i, 1) = v1.mPositions.y;
		fixed_vertices_coordinate(i, 2) = v1.mPositions.z;
	}	
}

void iCathMesh::setContrlVerticesCoordinate(const IntArray face_ids_constrol, const IntArray indexes ,
	const iv::vec3& offset,	Eigen::MatrixXd& fixed_vertices_coordinate,const int flag)
{	
	double b = sqrt(offset.x*offset.x + offset.y*offset.y + offset.z*offset.z);	
	// TODO 也可以根据鼠标偏移与控制点法向的夹角来决定b的正负
	if (offset.y < 0)// 根据鼠标的位移的y值决定b的正负，即凹陷还是突起
	{
		b = -b;
	}
	b = 0.5 * b;// 做一个中和，不会变形过大

    //构造狭窄，设置一些约束
	if (flag == 0)
	{	
		sum_b += b;
		if (sum_b < -mVesselRadius*0.7)//狭窄程度不能过大
		{
			b = 0;
		}
		else if (sum_b > 0)//必须是内缩不能外扩
		{
			b = 0;
		}
	}
	
	std::cout << "b=" << b << std::endl;
	std::cout << "mVesselRadius=" << mVesselRadius << std::endl;

	// 设置控制点固定位置约束
	for (int j = 0; j < face_ids_constrol.size(); j++)
	{
		Face f = mFaceMap[face_ids_constrol[j]];		
		for (int i = 0; i < 3; i++)
		{
			Vertex v = mVertexMap[f.mVerticesID[i]];			
			if (flag == 0)
			{
				int t = indexes[i + j * 3];
				fixed_vertices_coordinate(t, 0) = v.mPositions.x + v.mNormals.x *b;
				fixed_vertices_coordinate(t, 1) = v.mPositions.y + v.mNormals.y *b;
				fixed_vertices_coordinate(t, 2) = v.mPositions.z + v.mNormals.z *b;
			}
			else
			{
				int t = indexes[i + j * 3];
				fixed_vertices_coordinate(t, 0) = v.mPositions.x + f.mNormal.x *b;
				fixed_vertices_coordinate(t, 1) = v.mPositions.y + f.mNormal.y *b;
				fixed_vertices_coordinate(t, 2) = v.mPositions.z + f.mNormal.z *b;
			}
		}		
	}	
}

void iCathMesh::getIndexes(const Eigen::VectorXi vertexID_A, const IntArray faceID_B, IntArray& BindexInA)
{
	
	BindexInA.resize(3* faceID_B.size());
	
	for (int k = 0 ; k < faceID_B.size(); k++)
	{
		Face f = mFaceMap[faceID_B[k]];
		
		for (int j = 0; j < 3; j++)
		{			
			int VID = f.mVerticesID[j];
			int nPosition;
			std::vector <int>::iterator iElement = std::find(vertex_ids_all.begin(),
				vertex_ids_all.end(), VID);

			if (iElement != vertex_ids_all.end())			
				nPosition = std::distance(vertex_ids_all.begin(), iElement);		
			
			for (int i = 0; i < vertexID_A.size(); i++)
			{
				if (nPosition == vertexID_A(i))
				{
					BindexInA[j+k*3]=i;					
					break;
				}
			}
		}		
	}		
}

void iCathMesh::updateVerticesMap(Eigen::MatrixXd& update_vertices, IntArray v)
{
	for (size_t i = 0; i < v.size(); i++)
	{
		iv::vec3 new_position = iv::vec3(update_vertices(i, 0), update_vertices(i, 1), update_vertices(i, 2));		
		mVertexMap[v[i]].mPositions = new_position;			
	}
}




void iCathMesh::getInitMesh()
{
	mVertexMap.clear();
	mFaceMap.clear();
	for (size_t i = 0; i < mFaceMapInit.size(); i++)
	{
		mFaceMap[i] = mFaceMapInit[i];
		
	}
	for (size_t i = 0; i < mVertexMapInit.size(); i++)
	{		
		mVertexMap[i] = mVertexMapInit[i];
		mVertexMap[i].mPositions = mVertexMapInit[i].mPositions;
		mVertexMap[i].mNormals = mVertexMapInit[i].mNormals;		
	}

	GenAdjacency();
	CalcPerFaceNormal();
	CalcPerVertexNormal();
	SetDirty();
}

// TODO 在涉及构造肿瘤的时候，会出现异常
void iCathMesh::getBackupMesh()
{
	if (!mFaceMapBackup.empty())
	{
		mVertexMap.clear();
		mFaceMap.clear();

		FaceMap fm = mFaceMapBackup.back();
		mFaceMapBackup.pop_back();
		VertexMap vm = mVertexMapBackup.back();
		mVertexMapBackup.pop_back();

		for (size_t i = 0; i < fm.size(); i++)
		{
			mFaceMap[i] = fm[i];
		}

		for (size_t i = 0; i < vm.size(); i++)
		{
			mVertexMap[i] = vm[i];
			mVertexMap[i].mPositions = vm[i].mPositions;
			mVertexMap[i].mNormals = vm[i].mNormals;
		}
		std::cout<<"这里" << std::endl;
		GenAdjacency();
		CalcPerFaceNormal();
		CalcPerVertexNormal();
		SetDirty();
	}
	else
	{
		std::cout << "There is no previous mesh,stop type B!" << std::endl;
	}
	
}

void iCathMesh::clearSelectedFace()
{		
	face_ids_control.clear();
	face_ids_free.clear();
	face_ids_all.clear();
}

int iCathMesh::findCrossFaceId_InTumorFace(const iv::vec3& p)
{	
	for (int i = 0 ; i < m_TumorFace.size(); i++)
	{
		Face f = mFaceMap[m_TumorFace[i]];
		if (isPointInTriangle(p, f.mID))					
			return f.mID;		
	}
	return -1;
}


bool iCathMesh::isPointInTriangle(iv::vec3 P, int faceId)
{
	Face f = mFaceMap[faceId];
	iv::vec3 a = mVertexMap[f.mVerticesID[0]].mPositions;
	iv::vec3 b = mVertexMap[f.mVerticesID[1]].mPositions;
	iv::vec3 c = mVertexMap[f.mVerticesID[2]].mPositions;
	a -= P;
	b -= P;
	c -= P;
	float ab = iv::dot(a, b);
	float ac = iv::dot(a, c);
	float bc = iv::dot(b, c);
	float cc = iv::dot(c, c);
	float bb = iv::dot(b, b);
	if (bc * ac - cc * ab < 0.0f) return false;
	if (ab * bc - ac * bb < 0.0f) return false;
	return true;
}

void iCathMesh::getCenterOfGravity(int face_id, iv::vec3& p)
{
	iv::vec3 v1 = mVertexMap[mFaceMap[face_id].mVerticesID[0]].mPositions;
	iv::vec3 v2 = mVertexMap[mFaceMap[face_id].mVerticesID[1]].mPositions;
	iv::vec3 v3 = mVertexMap[mFaceMap[face_id].mVerticesID[2]].mPositions;

	p.x = (v1.x + v2.x + v3.x) / 3.0;
	p.y = (v1.y + v2.y + v3.y) / 3.0;
	p.z = (v1.z + v2.z + v3.z) / 3.0;
}

//TODO: t的取值需要根据具体模型调整
float iCathMesh::getVesselRadius(const iv::vec3 &p1, float t)
{
	UpdateCollider();
	std::vector<int> vCollideFace0,vCollideFace1, vCollideFace_larger;

	//TODO 下面三个函数中第2个参数可以根据具体模型调整，数值越大，范围越大
	getMoreFace(p1, t, vCollideFace0);//vCollideFace0中的面用来确定位置
	getMoreFace(p1, 4*t, vCollideFace1);//vCollideFace1中的面用来确定法向量
	getMoreFace(p1, 15*t, vCollideFace_larger); //从vCollideFace_larger中的面里找符合条件的面

	if ((vCollideFace0.size() == 0) || (vCollideFace1.size() == 0) || 
		(vCollideFace_larger.size() == 0))
	{
		std::cout << "vCollideFace size is zero" << std::endl;
		return 0.0f;
	}	

	iv::vec3 n_mean = iv::vec3(0,0,0);
	std::vector<iv::vec3> n_vector;
	iv::vec3 g = mFaceMap[vCollideFace0[0]].mGravity;
	
	// vCollideFace1中面的法向两两结合构成vCollideFace1.size()-1个平面
	// 求每个平面的法向，并保存在n_vector中
	for (int i = 0 ; i < vCollideFace1.size()-1;i++ )
	{
		Face &face1 = mFaceMap[vCollideFace1[i]];
		Face &face2 = mFaceMap[vCollideFace1[i+1]];
		iv::vec3 n_i = iv::normalize(iv::cross(face1.mNormal, face2.mNormal));
		n_vector.push_back(n_i);
	}
	for (int i = 0; i < vCollideFace1.size(); i++)	
		n_mean += n_vector[i];
	
	n_mean = iv::normalize(n_mean);//得到n_vector中向量的均值


	// 平面点法式方程：A(x-x0)+B(y-y0)+C(z-z0)=0，（A,B,C）是平面的法向
	// Ax-Ax0+By-By0+Cz-Cz0=0
	// Ax+By+Cz=Ax0+By0+Cz0
	// 令d = Ax0+By0+Cz0
	float d = -(n_mean.x*g.x + n_mean.y*g.y + n_mean.z*g.z);

	std::vector<int> circle_face_id;
	for (int i = 0; i < vCollideFace_larger.size(); i++)
	{
		Face f = mFaceMap[vCollideFace_larger[i]];
		// e=Ax+By+Cz
		float e = n_mean.x * f.mGravity.x + n_mean.y * f.mGravity.y + n_mean.z * f.mGravity.z;

		if (((e + d) >= -0.010) && ((e + d) <= 0.010))
		{
			circle_face_id.push_back(f.mID);
		}

	}
	
	if (circle_face_id.size() <= 1)
	{
		std::cout << "circle_face_id size is zero" << std::endl;
		return 0.0f;
	}

	iv::vec3 c(0.0f);
	float fradius = 0.0f;
	// 得到circle_face_id中所有面的中心点
	for (size_t i = 0; i < circle_face_id.size(); i++)
	{
		Face &rf = mFaceMap[circle_face_id[i]];
		c += rf.mGravity;
	}
	std::cout << "circle_face_id size ：" << circle_face_id.size() << std::endl;
	c /= static_cast<float>(circle_face_id.size());

	// circle_face_id中所有面的中心到中心点的距离求平均得到近似半径
	for (size_t i = 0; i < circle_face_id.size(); i++)
	{
		Face &rf = mFaceMap[circle_face_id[i]];
		fradius += iv::length(rf.mGravity - c);
	}
	fradius /= static_cast<float>(circle_face_id.size());

	return fradius;	
	
	//return 0.01f;
}


void iCathMesh::SelectLittleTumorFaces(iv::vec3 &p)
{
	clearSelectedFace();
	mVesselRadius = getVesselRadius(p,0.02);
	IntArray roi;
	getMoreFace(p, mVesselRadius, roi);

	for (int i = 0; i < roi.size(); i++)
	{
		Face f = mFaceMap[roi[i]];
		if (isPointInTriangle(p, f.mID))
		{
			face_ids_control.push_back(f.mID);//control的面
		}
	}

	face_ids_free = roi;//free的面	
	getMoreFace(p, mVesselRadius*1.5, face_ids_all);
	m_TumorFace.clear();
	m_TumorFace = face_ids_all;//总的面	
	
	
}

void iCathMesh::DrawRandomBending()
{
	int size_TumorFace = m_TumorFace.size();
	int size_Part = m_TumorFace_Part.size();
	
	IntArray c;

	//随机选取控制点
	for (int j = 0 ; j < 2 ;j ++) // j=0时，构造突起，j=1时，构造凹陷
	{
		clearSelectedFace();
		for (int i = 0; i < 3; i++) // 将肿瘤网格分成3部分
		{
			int iStart = size_Part / 3.0 * i;
			int iEnd = size_Part / 3.0 * (i + 1);
			int iRandom = getRandom(iStart,iEnd);
			int jRandom = getRandom(10 , m_TumorFace_Part[iRandom].size()-5);
			
			if (jRandom == -1)
			{				
				iRandom = m_TumorFace_Part[iRandom].size()-1;
			}

			int index_of_tumor = 0;
			for (size_t t = 0; t < iRandom; t++)
			{
				index_of_tumor += m_TumorFace_Part[t].size();
			}
			index_of_tumor += jRandom;
			
			int q = m_TumorFace[index_of_tumor];
			if (index_of_tumor >= m_TumorFace.size() || index_of_tumor < 0)
			{
				std::cout << "Triangles are not in the optional range1" << std::endl;
				continue;
			}
			if (q == -1)
			{
				std::cout << "Triangles are not in the optional range" << std::endl;
				continue;
				//return;
			}
			
			Face f1 = mFaceMap[q];
			face_ids_control.push_back(q);
			iv::vec3 p = f1.mGravity;
			
			getMoreFaceInTumorFace(q,mVesselRadius*0.5, face_ids_free);			
		}		

		doArapmodelingPrecompute_TumorMode();

		int b = 0;
		if (j == 1)// b的正负决定了突起还是凹陷
			b = -1;			
		else		
			b = 1;					
		
		//TODO 下面函数中偏移量可以调整，决定了构造肿瘤时表面的不规则形变的程度
		doDeform(iv::vec3(mVesselRadius*0.3f, mVesselRadius*0.3f*b, mVesselRadius*0.3f ), 1);
	}			
}

int iCathMesh::getRandom(int a, int b)
{
	if (b > (a+1))			
		return (rand() % (b - a)) + a;	
	else	
		return -1;	
}

bool iCathMesh::isEdge(int vertice_i, int vertice_j)
{
	bool flag = false;
	Vertex vi = mVertexMap[vertice_i];
	Vertex vj = mVertexMap[vertice_j];	

	std::vector<int>::iterator it1,it2;	

	it1 = std::find(vi.mNeighborVertexID.begin(), vi.mNeighborVertexID.end(), vj.mID);
	it2 = std::find(vj.mNeighborVertexID.begin(), vj.mNeighborVertexID.end(), vi.mID);

	if ((it1 != vi.mNeighborVertexID.end()) || (it2 != vj.mNeighborVertexID.end()))
	{
		flag = true;
	}
	return flag;
}

void iCathMesh::MeshSmothing(const IntArray faces_id, int times)
{
	std::cout << "MeshSmothing...................";
	IntArray vertices_id,border_index;
	getVIDByFID(faces_id, vertices_id);
	getBorderIndex(border_index, vertices_id);	
	int vertices_num = vertices_id.size();
	MatrixXf weight_;
	computeCotWeight(weight_, faces_id, vertices_id);
	int c = 0;
	while (c < times) {// 循环times次
		c++;
		for (int i = 0; i < vertices_num; i++)
		{
			float L_x = 0, L_y = 0, L_z = 0;
			Vertex v = mVertexMap[vertices_id[i]];
			int neighboe_num = v.mNeighborVertexID.size();

			for (int j = 0; j < neighboe_num; j++)
			{
				Vertex v_neighbor = mVertexMap[v.mNeighborVertexID[j]];
				std::vector <int>::iterator iElementFirst = find(vertices_id.begin(),
					vertices_id.end(), v_neighbor.mID);
				if (iElementFirst != vertices_id.end())
				{
					int t = std::distance(vertices_id.begin(), iElementFirst);
					L_x += (v_neighbor.mPositions.x - v.mPositions.x)* weight_(i, t);
					L_y += (v_neighbor.mPositions.y - v.mPositions.y)* weight_(i, t);
					L_z += (v_neighbor.mPositions.z - v.mPositions.z)* weight_(i, t);
				}
				
			}

			float new_x = L_x * 0.5 + v.mPositions.x;
			float new_y = L_y * 0.5 + v.mPositions.y;
			float new_z = L_z * 0.5 + v.mPositions.z;

			std::vector <int>::iterator iElement = find(border_index.begin(),
				border_index.end(), i);

			if (iElement == border_index.end())
			{
				mVertexMap[vertices_id[i]].mPositions.x = new_x;
				mVertexMap[vertices_id[i]].mPositions.y = new_y;
				mVertexMap[vertices_id[i]].mPositions.z = new_z;
			}
		}
	}
	std::cout << "done" << std::endl;
}


Eigen::Vector3d iCathMesh::ComputeCotangent(int face_id) {
	Eigen::Vector3d cotangent(0.0, 0.0, 0.0);
	std::vector<Eigen::Vector3d> vertices;

	vertices.resize(3);
	Face face = mFaceMap[face_id];
	for (int i = 0; i < 3; i++)
	{
		vertices[i](0) = mVertexMap[face.mVerticesID[i]].mPositions.x;
		vertices[i](1) = mVertexMap[face.mVerticesID[i]].mPositions.y;
		vertices[i](2) = mVertexMap[face.mVerticesID[i]].mPositions.z;
	}

	Eigen::Vector3d A = vertices[0];
	Eigen::Vector3d B = vertices[1];
	Eigen::Vector3d C = vertices[2];
	double a_squared = (B - C).squaredNorm();
	double b_squared = (C - A).squaredNorm();
	double c_squared = (A - B).squaredNorm();	
	double area = (B - A).cross(C - A).norm() / 2;	
	double four_area = 4 * area;
	cotangent(0) = (b_squared + c_squared - a_squared) / four_area;
	cotangent(1) = (c_squared + a_squared - b_squared) / four_area;
	cotangent(2) = (a_squared + b_squared - c_squared) / four_area;
	return cotangent;
}

void iCathMesh::computeCotWeight(Eigen::MatrixXf& weight_, IntArray face_ids,IntArray vertex_ids)
{
	int index_map[3][2] = { { 1, 2 },{ 2, 0 },{ 0, 1 } };
	int face_num = face_ids.size();	
	int vertex_num = vertex_ids.size();
	weight_.resize(vertex_num, vertex_num);
	weight_ = MatrixXf::Zero(vertex_num, vertex_num);

	std::vector<double> sum_weight = std::vector<double>(vertex_num);

	for (int f = 0; f < face_num; f++) {
		
		// Get the cotangent value with that face.
		Eigen::Vector3d cotangent = ComputeCotangent(face_ids[f]);
		// Loop over the three vertices within the same triangle.
		// i = 0 => A.
		// i = 1 => B.
		// i = 2 => C.
		for (int i = 0; i < 3; ++i) {
			// Indices of the two vertices in the edge corresponding to vertex i.					
			int first = mFaceMap[face_ids[f]].mVerticesID[index_map[i][0]];//一个点的id
			int second = mFaceMap[face_ids[f]].mVerticesID[index_map[i][1]];

			double cot = cotangent(i) ;
			std::vector <int>::iterator iElementFirst = find(vertex_ids.begin(),
				vertex_ids.end(), first);
			int nPositionFirst = std::distance(vertex_ids.begin(), iElementFirst);

			std::vector <int>::iterator iElementSecond = find(vertex_ids.begin(),
				vertex_ids.end(), second);
			int nPositionSecond = std::distance(vertex_ids.begin(), iElementSecond);
			
			double half_cot = cotangent(i) / 2.0;
			
			weight_(nPositionFirst, nPositionSecond) += half_cot;
			weight_(nPositionSecond, nPositionFirst) += half_cot;
			sum_weight[nPositionFirst] += half_cot;
			sum_weight[nPositionSecond] += half_cot;
			weight_(nPositionFirst, nPositionFirst) -= half_cot;
			weight_(nPositionSecond, nPositionSecond) -= half_cot;
		}
	}
	
	for (int i = 0; i < vertex_num; i++)	
		for (int j = 0; j < vertex_num; j++)					
			weight_(i, j) = weight_(i, j) / (sum_weight[i] );			
		
	

}


//border_vertices_index是在vertices_id中的下标
void iCathMesh::getBorderIndex(IntArray &border_vertices_index, IntArray vertices_id)
{	
	for (int i = 0 ; i < vertices_id.size(); i++)
	{
		Vertex v = mVertexMap[vertices_id[i]];
		IntArray neighborsVertice = v.mNeighborVertexID;
		bool flag = false;
		for (int j = 0; j < neighborsVertice.size(); j++)
		{
			std::vector <int>::iterator iElementSecond = find(vertices_id.begin(),
				vertices_id.end(), neighborsVertice[j]);
			if (iElementSecond == vertices_id.end())//点i的某个邻接面不在facesID中，点i是边界点
			{
				flag = true;
				break;
			}
		}
		if (flag == true)		
			border_vertices_index.push_back(i);		
	}
}

int iCathMesh::getNeighborVerticesNumber(IntArray vertices_id, int index)
{
	
	Vertex v = mVertexMap[vertices_id[index]];
	int vn = vertices_id.size();
	int count = 0;
	for (int j = 0; j < vn; j++)		
		if (isEdge(vertices_id[index], vertices_id[j]))		
			count++;	
	return count;
}

void iCathMesh::MeshOptimization(const IntArray faces_id, const IntArray fixed_vertices_id)
{
	std::cout << "MeshOptimization...................";

	IntArray vertices_id, border_index, a;
	getVIDByFID(faces_id, vertices_id);
	getBorderIndex(border_index, vertices_id);

	int vn = vertices_id.size();
	int vm = border_index.size();
	int vs = fixed_vertices_id.size();

	MatrixXf A(vn + vm + vs, vn);
	VectorXf conX(vn + vm + vs), conY(vn + vm + vs), conZ(vn + vm + vs);
	VectorXf X(vn), Y(vn), Z(vn);
	A = MatrixXf::Zero(vn + vm + vs, vn);
	conX = conY = conZ = VectorXf::Zero(vn + vm + vs);

	//compute A
	for (int i = 0; i < vn; i++)
	{
		int iNeighbor = getNeighborVerticesNumber(vertices_id, i);
		int count = 0;
		for (int j = 0; j < vn; j++)
		{
			if (i == j)			
				A(i, j) = -1;			
			else if (isEdge(vertices_id[i], vertices_id[j]))
			{
				count++;
				A(i, j) = 1.0f / (iNeighbor - 1);
			}
		}

		
	}
	for (int i = vn; i < vn + vm; i++)	
		A(i, border_index[i - vn]) = 1;
	
	for (int i = vn + vm; i < vn + vm + vs; i++)
	{
		std::vector <int>::iterator iElement = find(vertices_id.begin(),
			vertices_id.end(), fixed_vertices_id[i- vn - vm]);
		int iPosition = std::distance(vertices_id.begin(), iElement);
		A(i, iPosition) = 1;
	}

	//compute rhs
	MatrixXf weight_;

	computeCotWeight(weight_, faces_id, vertices_id);

	for (int i = 0; i < vn; i++)
	{
		for (int j = 0; j < vn; j++)
		{
			conX(i) += weight_(i, j) * mVertexMap[vertices_id[j]].mPositions.x;
			conY(i) += weight_(i, j) * mVertexMap[vertices_id[j]].mPositions.y;
			conZ(i) += weight_(i, j) * mVertexMap[vertices_id[j]].mPositions.z;
		}

	}

	for (int i = vn; i < vn + vm; i++)
	{
		conX(i) = mVertexMap[vertices_id[border_index[i - vn]]].mPositions.x;
		conY(i) = mVertexMap[vertices_id[border_index[i - vn]]].mPositions.y;
		conZ(i) = mVertexMap[vertices_id[border_index[i - vn]]].mPositions.z;
	}

	for (int i = vn + vm; i < vn + vm + vs; i++)
	{
		
		conX(i) = mVertexMap[fixed_vertices_id[i - vn - vm]].mPositions.x;
		conY(i) = mVertexMap[fixed_vertices_id[i - vn - vm]].mPositions.y;
		conZ(i) = mVertexMap[fixed_vertices_id[i - vn - vm]].mPositions.z;
	}


	//
	X = (A.transpose()*A).inverse()*A.transpose()*conX;
	Y = (A.transpose()*A).inverse()*A.transpose()*conY;
	Z = (A.transpose()*A).inverse()*A.transpose()*conZ;


	for (int t = 0 ;t < vn; t++)
	{
		std::vector <int>::iterator iElement = find(border_index.begin(),
		border_index.end(), t);
		if (iElement == border_index.end())
		{
			mVertexMap[vertices_id[t]].mPositions.x = X(t);
			mVertexMap[vertices_id[t]].mPositions.y = Y(t);
			mVertexMap[vertices_id[t]].mPositions.z = Z(t);
		}
	}
	std::cout << "done" << std::endl;
}


void iCathMesh::MeshSimplification(const IntArray faces_id)
{
	std::cout << "MeshSimplification...................";

	IntArray verticesID,faceID_temp,faceID_more;
	getVIDByFID(faces_id, verticesID);
	getMoreNeighber(faces_id, faceID_more);
	get_difference(faceID_more, faces_id, faceID_temp);

	MatrixXd V, OV;
	MatrixXi F, OF;

	V.resize(verticesID.size(), 3);
	F.resize(faces_id.size(), 3);

	
	//构造 F
	for (int i = 0; i < faces_id.size(); i++)
	{
		IntArray ids = mFaceMap[faces_id[i]].mVerticesID;

		std::vector <int>::iterator iElement0 = find(verticesID.begin(),
			verticesID.end(), ids[0]);
		int iPosition0 = std::distance(verticesID.begin(), iElement0);
		std::vector <int>::iterator iElement1 = find(verticesID.begin(),
			verticesID.end(), ids[1]);
		int iPosition1 = std::distance(verticesID.begin(), iElement1);
		std::vector <int>::iterator iElement2 = find(verticesID.begin(),
			verticesID.end(), ids[2]);
		int iPosition2 = std::distance(verticesID.begin(), iElement2);

		F(i, 0) = iPosition0;
		F(i, 1) = iPosition1;
		F(i, 2) = iPosition2;
	}
	
	//构造 V
	for (int i = 0; i < verticesID.size(); i++)
	{
		V(i, 0) = mVertexMap[verticesID[i]].mPositions.x;
		V(i, 1) = mVertexMap[verticesID[i]].mPositions.y;
		V(i, 2) = mVertexMap[verticesID[i]].mPositions.z;
	}
	
	SimplifyObject obj;
	obj.Load(V, F);
	obj.simpify(0.3);//简化主函数
	obj.Save(OV, OF);	

	IntArray mapper; // 从点在OV中的位置到点的mID的映射
	std::map<int, bool>;
	IntArray temp_vertice, vertice_border;
	getVIDByFID(faceID_temp, temp_vertice);
	getBorderIndex(vertice_border, temp_vertice);
	
	//删除'facesID' 中的面和其中顶点。
	for (int i = 0; i < faces_id.size(); ++i)
	{
		int iID = faces_id[i];
		mFaceMap.erase(iID);
	}
	std::vector<int> temp;
	for (int i = 0 ; i < vertice_border.size();i++)
	{
		temp.push_back(temp_vertice[vertice_border[i]]);
	}
	for (int i = 0 ; i < verticesID.size(); i++)
	{
		int iID = verticesID[i];

		std::vector <int>::iterator iElement0 = find(temp.begin(),
			temp.end(), iID);
		int iPosition0 = std::distance(temp.begin(), iElement0);

		if (iElement0 == temp.end())//not boundary vertice,delete
		{
			mVertexMap.erase(iID);
		}
	}
			
	//根据简化结果构造新的点
	for (int i = 0; i < OV.rows(); i++)
	{
		iv::vec3 new_position = iv::vec3(OV(i, 0), OV(i, 1), OV(i, 2));

		bool flag = false;
		int id = 0;

		// 如果OV中的顶点坐标与输入网格的边界坐标相同，则不在构建新的点，把那个边界点的ID加入到mapper中
		for (int j = 0 ;j < vertice_border.size();j++)
		{
			if (mVertexMap[temp_vertice[vertice_border[j]]].mPositions == new_position)
			{
				id = mVertexMap[temp_vertice[vertice_border[j]]].mID;
				flag = true;
			}
		}

		if (flag == false)
		{
			Vertex nv;
			nv.mID = GenNextNewVertexID();
			nv.mPositions = new_position;
			mVertexMap[nv.mID] = nv;
			mapper.push_back(nv.mID);
		}
		else
		{
			mapper.push_back(id);
		}
	}

	
	//根据简化结果构造新的面
	m_TumorFace.clear();
	for (int i = 0; i < OF.rows(); i++)
	{
		iCathMesh::FaceMap::reverse_iterator itr = mFaceMap.rbegin();
		
		iCathMesh::Face newFace1;
		newFace1.mID = itr->first + 1;
		newFace1.mVerticesID.push_back(mapper[OF(i, 0)]);
		newFace1.mVerticesID.push_back(mapper[OF(i, 1)]);
		newFace1.mVerticesID.push_back(mapper[OF(i, 2)]);
		mFaceMap[newFace1.mID] = newFace1;
		m_TumorFace.push_back(newFace1.mID);
		
	}	

	//GenAdjacency
	InValidAdjacencyInfo();		
	GenAdjacency();
	
	//CalcNormal
	CalcPerTumorFaceNormalForSim();	
	CalcPerTumorVertexNormalForSim();
	CalcPerFaceGravity();
	
	std::cout << "done" << std::endl;
}


