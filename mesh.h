#pragma once
#include <math.h>
#include "AbstractGraph.h"
#include "aMesh.h"
#include "SiMath.h"
class Mesh:public AbstractGraph
{
public:
	std::map<int,iCathMesh::Vertex> mVertMap;	
	std::map<int,iCathMesh::Face> mFaceMap;
	std::vector<std::vector<int>> AdjacentVerticesPerVertex;
public:
	int AddVertex(iCathMesh::Vertex& toAdd)
	{
		int index = mVertMap.size();
		mVertMap[index] = toAdd;
		return index;
	}

	
	int AddFace(iCathMesh::Face& toAdd)
	{
		int index = mFaceMap.size();
		mFaceMap[index] = toAdd;
		return index;
	}

	float getDistance(int p0Index, int p1Index)
	{
		/*iCathMesh::Vertex v0 = mVertMap[p0Index];
		iCathMesh::Vertex v1 = mVertMap[p1Index];
		float x = v1.mPositions.x - v0.mPositions.x;
		float y = v1.mPositions.y - v0.mPositions.y;
		float z = v1.mPositions.z - v0.mPositions.z;
		return sqrt(x*x + y*y + z*z);*/

		iCathMesh::Face f0 = mFaceMap[p0Index];
		iCathMesh::Face f1 = mFaceMap[p1Index];
		float x = f1.mGravity.x - f0.mGravity.x;
		float y = f1.mGravity.y - f0.mGravity.y;
		float z = f1.mGravity.z - f0.mGravity.z;
		return sqrt(x*x + y*y + z*z);
	}


	void CaculateAdjacentVerticesPerVertex()
	{
		/*AdjacentVerticesPerVertex.resize(mVertMap.size());
		for (int i = 0; i < mVertMap.size(); i++)
		{
			AdjacentVerticesPerVertex[i] = GetNeighbourList(i);
		}*/
		AdjacentVerticesPerVertex.resize(mFaceMap.size());
		for (int i = 0; i < mFaceMap.size(); i++)
		{
			AdjacentVerticesPerVertex[i] = GetNeighbourList(i);
		}
	}

public:
	Mesh(std::map<int, iCathMesh::Vertex> v, std::map<int, iCathMesh::Face> f) 
	{
		mVertMap = v;
		mFaceMap = f;

		/*AdjacentVerticesPerVertex.resize(mVertMap.size());
		for (int i = 0 ; i < mVertMap.size(); i++)
		{
			AdjacentVerticesPerVertex[i] = GetNeighbourList(i);
		}*/

		AdjacentVerticesPerVertex.resize(mFaceMap.size());
		for (int i = 0 ; i < mFaceMap.size(); i++)
		{
			AdjacentVerticesPerVertex[i] = GetNeighbourList(i);
		}
		
	};
	~Mesh() {};
	float GetWeight(int p0Index, int p1Index)
	{
		if (p0Index == p1Index)
			return 0;
		return getDistance(p0Index,p1Index);
	}


	float GetEvaDistance(int p0Index, int p1Index)
	{
		return getDistance(p0Index, p1Index);
	}
	std::vector<int>& GetNeighbourList(int index)
	{
		/*		
		return mVertMap[index].mNeighborVertexID;*/
		
		return mFaceMap[index].mNeighborFaceID;
		
	}
	int GetNodeCount()
	{
		return (int)mFaceMap.size();
	}




};
