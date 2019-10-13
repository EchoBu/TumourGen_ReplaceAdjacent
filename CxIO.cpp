#include "stdafx.h"
#include "CxIO.h"
#include "aMesh.h"
#include "Collider.h"
#include "ObjParser.h"
#include "TextFileDataStream.h"
#include <string>
#include <fstream>
#include <set>
#include <igl/loop.h>
#include <igl/writeOBJ.h>


iCathMesh* CxImporter::LoadObjFile( const char* filePath, int postFlags )
{
	iCathMesh* pMesh = new iCathMesh;
	iCathMesh* pMesh1 = new iCathMesh;
	if (!pMesh)
		return 0;

	
	ObjParser objParser(filePath, pMesh);
	

	/*if (pMesh->mVertexMap.size() < 3000)
	{
		Eigen::MatrixXi  F;
		Eigen::MatrixXd  V;
		pMesh->getFaceMatrix(F);
		pMesh->getVexticesMatrix(V);
		
		igl::loop(Eigen::MatrixXd(V), Eigen::MatrixXi(F), V, F);
		igl::writeOBJ("aorta.obj", V, F);
		filePath = "aorta.obj";
		ObjParser objParser(filePath, pMesh1);
		pMesh = pMesh1;
		
	}*/

	

	

	pMesh->GenAdjacency();
	pMesh->CalcPerFaceNormal();
	pMesh->CalcPerVertexNormal();
	pMesh->CalcPerFaceGravity();
	

	pMesh->mFaceMapBackup.push_back(pMesh->mFaceMap) ;
	pMesh->mFaceMapInit = pMesh->mFaceMap;
	pMesh->mVertexMapBackup.push_back(pMesh->mVertexMap) ;
	pMesh->mVertexMapInit = pMesh->mVertexMap;
	
	return pMesh;
}

iCathMesh* CxImporter::LoadSTLFile( const char* filePath, int postFlags )
{
	std::string filename(filePath);
	std::ifstream stlFile;
	stlFile.open(filePath,std::ios::ate | std::ios::binary);

	if(!stlFile.is_open())
	{
		printf("Failed to open file!\n");
		return NULL;
	}

	size_t fileLength = stlFile.tellg();

	std::vector<char> dataArray;

	dataArray.resize(fileLength);

	stlFile.seekg(0,std::ios::beg);

	stlFile.read(&dataArray[0],fileLength);

	stlFile.close();

	char* ptr = &dataArray[0] + 80;

	int faceCnt = *((int*)ptr);

	ptr += 4;


	iCathMesh* pMesh = new iCathMesh;


	typedef std::set<iCathMesh::Vertex> VertexSet;
	VertexSet tmpVertexSet;
	std::pair<VertexSet::iterator,bool> pair_set;

	int nIndex = 0;
	int nCnt = 0;
	for(int i = 0 ; i < faceCnt ; ++i)
	{
		float normal[3];
		memcpy(normal,ptr,4*3);

		ptr += 12;

		iCathMesh::Face face;
		face.mNormal = iv::vec3(normal);
		face.mID = i;

		for(int j = 0 ; j < 3 ; ++j)
		{
			float vert[3];
			memcpy(vert,ptr,4*3);
			ptr += 12;
			iCathMesh::Vertex v;
			v.mPositions = iv::vec3(vert);
			v.mID = pMesh->mVertexMap.size();
			pair_set = tmpVertexSet.insert(v);

			if(pair_set.second == true)
			{
				iCathMesh::Vertex& rv = const_cast<iCathMesh::Vertex&>(*pair_set.first);
				rv.mID = nIndex++;
				pMesh->mVertexMap[v.mID] = v;
			}

			VertexSet::iterator it = tmpVertexSet.find(v);
			if(it!=tmpVertexSet.end())
			{
				assert(it->mID >= 0);
				face.mVerticesID.push_back(it->mID);
			}
		}
		face.mID = pMesh->mFaceMap.size();
		pMesh->mFaceMap[face.mID] = face;
		ptr += 2;
	}

	pMesh->CalcPerFaceGravity();

	pMesh->mFaceMapBackup.push_back(pMesh->mFaceMap);
	pMesh->mFaceMapInit = pMesh->mFaceMap;
	pMesh->mVertexMapBackup.push_back(pMesh->mVertexMap);
	pMesh->mVertexMapInit = pMesh->mVertexMap;

	return pMesh;
}

iCathMesh* CxImporter::LoadVMBinFile( const char* filePath,int postFlags )
{

	std::ifstream is(filePath, std::ifstream::binary | std::ios::ate);
	if (!is ) return NULL;

	assert ( 4 == sizeof(float) );
	assert ( 4 == sizeof(unsigned int) );

	std::vector<char> fileData;

	int fileLength = is.tellg();

	fileData.resize(fileLength);

	is.seekg(0,std::ios::beg);

	is.read(&fileData[0],fileLength);

	char* ptr = &fileData[0];

	unsigned int nv = 0;
	memcpy(&nv,ptr,4);
	ptr += 4;
	unsigned int nf = 0;
	memcpy(&nf,ptr,4);
	ptr += 4;

	iCathMesh* pMesh = new iCathMesh;

	unsigned int v3Size = sizeof(iv::vec3);

	for ( unsigned int i=0; i<nv; ++i )
	{
		iv::vec3 v;
		memcpy(&v,ptr,v3Size);
		ptr += v3Size;
		iCathMesh::Vertex vertex;
		vertex.mPositions = v;
		vertex.mID = pMesh->mVertexMap.size();
		pMesh->mVertexMap[vertex.mID] = vertex;
	}

	for ( unsigned int i=0; i<nv; ++i )
	{

		iv::vec3 n;
		memcpy(&n,ptr,v3Size);
		ptr += v3Size;
		pMesh->mVertexMap[i].mNormals = n;
	}

	for ( unsigned int j=0; j<nf; ++j )
	{
		unsigned int f[3];
		memcpy(f,ptr,12);
		ptr += 12;
		iCathMesh::Face face;
		for(int i = 0 ; i < 3 ; i++)
			face.mVerticesID.push_back(f[i]);
		face.mID = pMesh->mFaceMap.size();
		pMesh->mFaceMap[face.mID] = face;
	}

	if(pMesh->m_pCollider)
		pMesh->m_pCollider.reset();
	pMesh->m_pCollider = std::shared_ptr<CxCollisionTest>(new CxCollisionTest);
	pMesh->m_pCollider->Init(pMesh);

	printf("%d vertex loaded!\n",nv);
	printf("%d face loaded!\n",nf);

	return pMesh;
}

iCathMesh* CxImporter::LoadVMBinFileEx( const char* filePath,int postFlags /*= 0*/ )
{
	std::ifstream is(filePath, std::ifstream::binary | std::ios::ate);
	if (!is ) return NULL;

	assert ( 4 == sizeof(float) );
	assert ( 4 == sizeof(unsigned int) );

	std::vector<char> fileData;

	int fileLength = is.tellg();

	fileData.resize(fileLength);

	is.seekg(0,std::ios::beg);

	is.read(&fileData[0],fileLength);

	char* ptr = &fileData[0];

	unsigned int nv = 0;
	memcpy(&nv,ptr,4);
	ptr += 4;
	unsigned int nf = 0;
	memcpy(&nf,ptr,4);
	ptr += 4;

	iCathMesh* pMesh = new iCathMesh;

	unsigned int v3Size = sizeof(iv::vec3);

	for ( unsigned int i=0; i<nv; ++i )
	{
		iv::vec3 v;
		memcpy(&v,ptr,v3Size);
		ptr += v3Size;
		iCathMesh::Vertex vertex;
		vertex.mPositions = v;
		vertex.mID = i;
		pMesh->mVertexMap[i] = vertex;
	}

	for ( unsigned int i=0; i<nv; ++i )
	{
		iv::vec3 n;
		memcpy(&n,ptr,v3Size);
		ptr += v3Size;
		pMesh->mVertexMap[i].mNormals = n;
	}

	for ( unsigned int j=0; j<nf; ++j )
	{
		unsigned int f[3];
		memcpy(f,ptr,12);
		ptr += 12;
		iCathMesh::Face face;
		for(int i = 0 ; i < 3 ; i++)
			face.mVerticesID.push_back(f[i]);
		pMesh->mFaceMap[j] = face;
	}
	return pMesh;
}

iCathMesh* CxImporter::LoadSVMBinFile( const char* filePath )
{
	std::ifstream is(filePath, std::ifstream::binary | std::ios::ate);
	if (!is ) return NULL;

	assert ( 4 == sizeof(float) );
	assert ( 4 == sizeof(unsigned int) );

	std::vector<char> fileData;

	int fileLength = is.tellg();

	fileData.resize(fileLength);

	is.seekg(0,std::ios::beg);

	is.read(&fileData[0],fileLength);

	const char* ptr = &fileData[0];

	if(!VerifySVMFile(ptr,fileLength))
	{
		printf("Falied To Import .SVM File.\n");
		return 0;
	}

	iCathMesh::MeshHeader header = *(reinterpret_cast<const iCathMesh::MeshHeader*>(ptr));

	int iVertex = header.iVertexCount;
	int iFace	= header.iFaceCount;

	printf("SVM File Information : *****************\n");
	printf("Vertex Count : %d.\n",header.iVertexCount);
	printf("Face Count : %d.\n",header.iFaceCount);
	printf("Tumor Count : %d.\n",header.iTumorCount);
	printf("Narrow Count : %d.\n",header.iNarrowCount);
	printf("*****************************************\n");

	ptr += sizeof(iCathMesh::MeshHeader);

	iCathMesh* pMesh = new iCathMesh;

	int v3Size = sizeof(iv::vec3);

	for (int i=0; i < iVertex; ++i )
	{
		iv::vec3 v;
		memcpy(&v,ptr,v3Size);
		ptr += v3Size;
		iCathMesh::Vertex vertex;
		vertex.mPositions = v;
		vertex.mID = i;
		pMesh->mVertexMap[i] = vertex;
	}

	for ( int i=0; i<iVertex; ++i )
	{
		iv::vec3 n;
		memcpy(&n,ptr,v3Size);
		ptr += v3Size;
		pMesh->mVertexMap[i].mNormals = n;
	}

	for ( int j=0; j<iFace; ++j )
	{
		int f[3];
		memcpy(f,ptr,12);
		ptr += 12;
		iCathMesh::Face face;
		face.mID = j;
		for(int i = 0 ; i < 3 ; i++)
		{
			if(f[i] < 0)
				printf("shit!\n");
			face.mVerticesID.push_back(f[i]);
		}
		pMesh->mFaceMap[j] = face;
	}

	// Load Tumor Information
	for(int i = 0; i < header.iTumorCount; ++i)
	{
		iCathMesh::TumorInfo tumorInfo = *(reinterpret_cast<const iCathMesh::TumorInfo*>(ptr));

		printf("Tumor #%d: Center : (%f,%f,%f). Radius : %f.\n",i,tumorInfo.vCenter.x,
			tumorInfo.vCenter.y,
			tumorInfo.vCenter.z,
			tumorInfo.fRadius);

		ptr += sizeof(iCathMesh::TumorInfo);
		iCathMesh::FaceSet tumorFaces;
		for(int j = 0; j < tumorInfo.iFaceCount; ++j)
		{
			int iFaceID = *(reinterpret_cast<const int*>(ptr));
			ptr += sizeof(int);
			tumorFaces.insert(iFaceID);
		}
		pMesh->mTumors.push_back(tumorFaces);
		pMesh->mTumorInfos.push_back(tumorInfo);
	}

	// Load Narrow Information/
	for(int i = 0; i < header.iNarrowCount; ++i)
	{
		iCathMesh::NarrowInfo narrowInfo = *(reinterpret_cast<const iCathMesh::NarrowInfo*>(ptr));
		ptr += sizeof(iCathMesh::NarrowInfo);
		iCathMesh::NarrowMap narrowMap;
		for(int j = 0; j < narrowInfo.iVertexCount; ++j)
		{
			int iVertexID = *(reinterpret_cast<const int*>(ptr));
			ptr += sizeof(int);
			iv::vec3 iHealthPos = *(reinterpret_cast<const iv::vec3*>(ptr));
			ptr += sizeof(iv::vec3);
			narrowMap[iVertexID] = iHealthPos;
		}
		pMesh->mNarrows.push_back(narrowMap);
	}

	if(pMesh->m_pCollider)
		pMesh->m_pCollider.reset();
	pMesh->m_pCollider = std::shared_ptr<CxCollisionTest>(new CxCollisionTest);
	pMesh->m_pCollider->Init(pMesh);

	return pMesh;
}

bool CxImporter::VerifySVMFile( const char *pData,int iBytes )
{
	const char *ptr = pData;

	iCathMesh::MeshHeader header = *(reinterpret_cast<const iCathMesh::MeshHeader*>(ptr));
	
	int iHeaderBytes = sizeof(iCathMesh::MeshHeader);
	ptr += iHeaderBytes;

	int iMeshBytes = header.iFaceCount * 3 * sizeof(int) + header.iVertexCount * sizeof(iv::vec3) * 2;
	ptr += iMeshBytes;

	int iTumorBytes = 0;
	for(int i = 0; i < header.iTumorCount; ++i)
	{
		iCathMesh::TumorInfo tumorInfo = *(reinterpret_cast<const iCathMesh::TumorInfo*>(ptr));
		if(tumorInfo.eValid != iCathMesh::TUMOR_VALID)
		{
			printf("SVM File Verification Failed.............Invalid Tumor Info.\n");
			return false;
		}
		int iEntryBytes = tumorInfo.iFaceCount * sizeof(int) + sizeof(iCathMesh::TumorInfo);
		iTumorBytes += iEntryBytes;
		ptr += iEntryBytes;
	}

	int iNarrowBytes = 0;
	for(int i = 0; i < header.iNarrowCount; ++i)
	{
		iCathMesh::NarrowInfo narrowInfo = *(reinterpret_cast<const iCathMesh::NarrowInfo*>(ptr));
		if(narrowInfo.eValid != iCathMesh::NARROW_VALID)
		{
			printf("SVM File Verification Failed.............Invalid Narrow Info.\n");
			return false;
		}
		int iEntryBytes = narrowInfo.iVertexCount * (sizeof(int) + sizeof(iv::vec3)) + sizeof(iCathMesh::NarrowInfo);
		iNarrowBytes += iEntryBytes;
		ptr += iEntryBytes;
	}
		
	if((iNarrowBytes + iTumorBytes + iMeshBytes + iHeaderBytes) != iBytes)
	{
		printf("SVM File Verification Failed.............Invalid File Length.");
		return false;
	}
	else
		return true;

}

iCathMesh* CxImporter::LoadNxVMBinFile( const char* filePath )
{
	std::ifstream is(filePath, std::ifstream::binary | std::ios::ate);
	if (!is ) return NULL;

	assert ( 4 == sizeof(float) );
	assert ( 4 == sizeof(unsigned int) );

	std::vector<char> fileData;

	int fileLength = is.tellg();

	fileData.resize(fileLength);

	is.seekg(0,std::ios::beg);

	is.read(&fileData[0],fileLength);

	char* ptr = &fileData[0];

	unsigned int nv = 0;
	memcpy(&nv,ptr,4);
	ptr += 4;
	unsigned int nf = 0;
	memcpy(&nf,ptr,4);
	ptr += 4;

	iCathMesh* pMesh = new iCathMesh;

	unsigned int v3Size = sizeof(iv::vec3);

	for ( unsigned int i=0; i<nv; ++i )
	{
		iv::vec3 v;
		memcpy(&v,ptr,v3Size);
		ptr += v3Size;
		iCathMesh::Vertex vertex;
		vertex.mPositions = v;
		pMesh->mVertexMap[i] = vertex;
	}

	for ( unsigned int i=0; i<nv; ++i )
	{

		iv::vec3 n;
		memcpy(&n,ptr,v3Size);
		ptr += v3Size;
		pMesh->mVertexMap[i].mNormals = n;
	}

	for ( unsigned int j=0; j<nf; ++j )
	{
		unsigned int f[3];
		memcpy(f,ptr,12);
		ptr += 12;
		iCathMesh::Face face;
		face.mID = j;
		for(int i = 0 ; i < 3 ; i++)
			face.mVerticesID.push_back(f[i]);
		pMesh->mFaceMap[j] = face;
	}

	// 载入病变之前健康的顶点信息
	int narrowedVertexCnt = *(int*)ptr;
	ptr += 4;

	int iCathPointSize = sizeof(iCathPoint);

	printf("NarrowedVertexCnt:%d.\n",narrowedVertexCnt);

	for(int i = 0; i < narrowedVertexCnt; ++i )
	{
		iCathPoint icp = *((iCathPoint*)ptr);
		pMesh->mVertexMap[icp.mID].mPositions = icp.mHealthPosition;
		ptr += iCathPointSize;
	}

	pMesh->UpdateCollider();

	return pMesh;
}

void CxImporter::loadNarFile(const char* filePath)
{
	std::ifstream narFile(filePath, std::ifstream::binary | std::ios::ate);
	if (!narFile.is_open())
		return;

	std::vector<char> fileData;

	int fileLength = narFile.tellg();

	fileData.resize(fileLength);
	narFile.seekg(0,std::ios::beg);
	narFile.read(&fileData[0],fileLength);
	narFile.close();

	char* ptr = &fileData[0];

	iCathMesh::NarrowInfo narInfo;
	narInfo.eValid = *(iCathMesh::Validation*)ptr;
	ptr += sizeof(iCathMesh::Validation);
	narInfo.iVertexCount = *(int*)ptr;
	ptr += sizeof(int);
	for(int i = 0; i < narInfo.iVertexCount; ++i)
	{
		int index = *(int*)ptr;
		ptr += sizeof(int);
		iv::vec3 p = *(iv::vec3*)ptr;
		ptr += sizeof(iv::vec3);
		printf("index:%d, p:(%f,%f,%f)\n",index,p.x,p.y,p.z);
	}
}

void CxExporter::SaveAsBinSVM(iCathMesh* pMesh,const char* filePath )
{
	if(!pMesh)
		return;
	std::vector<iCathMesh::Face> io_vFaces;
	std::vector<iCathMesh::Vertex> io_vVertex;
	std::vector<std::vector<int> > io_vTumors;
	std::vector<std::map<int,iv::vec3> > io_vNarrows;

	std::map<int,int> mOld2New;
	std::map<int,int>::iterator itrX;
	std::map<int,int> mFaceOld2New;
	std::map<int,int>::iterator itrFace;

	for(iCathMesh::FaceMap::iterator itr = pMesh->mFaceMap.begin(); itr!= pMesh->mFaceMap.end(); ++itr)
	{
		iCathMesh::Face nf;
		nf.mID = io_vFaces.size();
		const iCathMesh::Face& rFace = itr->second;
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
				iCathMesh::Vertex nv = pMesh->mVertexMap[iV];
				nv.mID = io_vVertex.size();
				io_vVertex.push_back(nv);
				nf.mVerticesID.push_back(nv.mID);
				mOld2New[iV] = nv.mID;
			}
		}
		mFaceOld2New[rFace.mID] = nf.mID;

		io_vFaces.push_back(nf);
	}

	// Save Tumor Parts.
	for(iCathMesh::TumorArray::iterator itr = pMesh->mTumors.begin(); itr != pMesh->mTumors.end(); ++itr)
	{
		std::vector<int> tumor;
		for(iCathMesh::FaceSet::iterator jtr = itr->begin(); jtr != itr->end(); ++jtr)
		{
			itrFace = mFaceOld2New.find(*jtr);
			if(itrFace == mFaceOld2New.end())
			{
				printf("Error In Export Tumor Info. Invalid Face Index.\n");
				return;
			}
			tumor.push_back(itrFace->second);
		}
		if(!tumor.empty())
			io_vTumors.push_back(tumor);
	}

	// Save Narrow Parts
	for(iCathMesh::NarrowArray::iterator itr = pMesh->mNarrows.begin(); itr != pMesh->mNarrows.end(); ++itr)
	{
		std::map<int,iv::vec3> narrow;
		for(iCathMesh::NarrowMap::iterator jtr = itr->begin(); jtr != itr->end(); ++jtr)
		{
			itrX = mOld2New.find(jtr->first);
			if(itrX == mOld2New.end())
			{
				printf("Error in Export Narrow Info. Invalid Vertex Index.\n");
				return;
			}
			narrow[itrX->second] = jtr->second;
		}
		if(!narrow.empty())
			io_vNarrows.push_back(narrow);
	}


	iCathMesh::MeshHeader header;
	header.iFaceCount = io_vFaces.size();
	header.iVertexCount = io_vVertex.size();
	header.iTumorCount = io_vTumors.size();
	header.iNarrowCount = io_vNarrows.size();

	// Header + Faces + Vertices + Normals + n * (TumorInfo + TumorFaceIDs) + m * (NarrowInfo + NarrowFaceIDs);
	int iFileLenthBytes = sizeof(iCathMesh::MeshHeader) + header.iFaceCount * 3 * sizeof(int);
	iFileLenthBytes += header.iVertexCount * sizeof(iv::vec3) * 2;

	for(int i = 0; i < io_vTumors.size(); ++i)
	{
		iFileLenthBytes += sizeof(iCathMesh::TumorInfo);
		iFileLenthBytes += io_vTumors[i].size() * sizeof(int);
	}
	for(int i = 0; i < io_vNarrows.size(); ++i)
	{
		iFileLenthBytes += sizeof(iCathMesh::NarrowInfo);
		iFileLenthBytes += io_vNarrows[i].size() * (sizeof(int) + sizeof(iv::vec3));
	}



	//!!!!!!!!!!!!!! The real save op. starts here!
	std::vector<char> dataBuffer;
	dataBuffer.resize(iFileLenthBytes);

	char *pCur = &dataBuffer[0];
	
	memcpy(pCur,&header,sizeof(iCathMesh::MeshHeader));
	pCur += sizeof(iCathMesh::MeshHeader);
	
	for(int i = 0; i < header.iVertexCount; ++i)
	{
		const iCathMesh::Vertex& rV = io_vVertex[i];
		memcpy(pCur,&rV.mPositions,sizeof(iv::vec3));
		pCur += sizeof(iv::vec3);
	}
	for(int i = 0 ; i < header.iVertexCount; ++i)
	{
		const iCathMesh::Vertex& rV = io_vVertex[i];
		memcpy(pCur,&rV.mNormals,sizeof(iv::vec3));
		pCur += sizeof(iv::vec3);
	}
	for(int i = 0; i < header.iFaceCount; ++i)
	{
		const iCathMesh::Face &rf = io_vFaces[i];

		for(int j = 0; j < rf.mVerticesID.size(); ++j)
		{
			if(rf.mVerticesID[j] < 0)
				printf("shit!\n");
		}

		memcpy(pCur,&rf.mVerticesID[0],3 * sizeof(int));
		pCur += 3 * sizeof(int);
	}

	// Tumor Extra Info.......................
	for(int i = 0; i < header.iTumorCount; ++i)
	{
		iCathMesh::TumorInfo tumorInfo;
		tumorInfo.eValid = iCathMesh::TUMOR_VALID;
		tumorInfo.iFaceCount = io_vTumors[i].size();
		tumorInfo.vCenter = pMesh->mTumorInfos[i].vCenter;
		tumorInfo.fRadius = pMesh->mTumorInfos[i].fRadius;
		memcpy(pCur,&tumorInfo,sizeof(iCathMesh::TumorInfo));
		pCur += sizeof(iCathMesh::TumorInfo);
		memcpy(pCur,&io_vTumors[i][0],sizeof(int) * tumorInfo.iFaceCount);
		pCur += tumorInfo.iFaceCount * sizeof(int);
	}

	// Narrow Extra Info.....................
	for(int i = 0; i < header.iNarrowCount; ++i)
	{
		iCathMesh::NarrowInfo narrowInfo;
		narrowInfo.eValid = iCathMesh::NARROW_VALID;
		narrowInfo.iVertexCount = io_vNarrows[i].size();
		memcpy(pCur,&narrowInfo,sizeof(iCathMesh::NarrowInfo));
		pCur += sizeof(iCathMesh::NarrowInfo);
		for(std::map<int,iv::vec3>::iterator itr = io_vNarrows[i].begin(); itr != io_vNarrows[i].end(); ++itr)
		{
			memcpy(pCur,&itr->first,sizeof(int));
			pCur += sizeof(int);
			memcpy(pCur,&itr->second,sizeof(iv::vec3));
			pCur += sizeof(iv::vec3);
		}
	}

	std::ofstream oFile(filePath,std::ios::binary);
	oFile.write(&dataBuffer[0],iFileLenthBytes);
	oFile.close();

	printf("Save Succeeded!\n");
}

void CxExporter::SaveAs_VM_Ex(iCathMesh* pMesh,const char* filePath)
{
	if(!pMesh)
		return;
	std::vector<iCathMesh::Face> io_vFaces;
	std::vector<iCathMesh::Vertex> io_vVertex;
	std::vector<std::vector<int> > io_vTumors;
	std::vector<std::map<int,iv::vec3> > io_vNarrows;

	std::map<int,int> mOld2New;
	std::map<int,int>::iterator itrX;
	std::map<int,int> mFaceOld2New;
	std::map<int,int>::iterator itrFace;

	for(iCathMesh::FaceMap::iterator itr = pMesh->mFaceMap.begin(); itr!= pMesh->mFaceMap.end(); ++itr)
	{
		iCathMesh::Face nf;
		nf.mID = io_vFaces.size();
		const iCathMesh::Face& rFace = itr->second;
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
				iCathMesh::Vertex nv = pMesh->mVertexMap[iV];
				nv.mID = io_vVertex.size();
				io_vVertex.push_back(nv);
				nf.mVerticesID.push_back(nv.mID);
				mOld2New[iV] = nv.mID;
			}
		}
		mFaceOld2New[rFace.mID] = nf.mID;

		io_vFaces.push_back(nf);
	}

	// Save Narrow Parts
	for(iCathMesh::NarrowArray::iterator itr = pMesh->mNarrows.begin(); itr != pMesh->mNarrows.end(); ++itr)
	{
		std::map<int,iv::vec3> narrow;
		for(iCathMesh::NarrowMap::iterator jtr = itr->begin(); jtr != itr->end(); ++jtr)
		{
			itrX = mOld2New.find(jtr->first);
			if(itrX == mOld2New.end())
			{
				printf("Error in Export Narrow Info. Invalid Vertex Index.\n");
				return;
			}
			narrow[itrX->second] = jtr->second;
		}
		if(!narrow.empty())
			io_vNarrows.push_back(narrow);
	}


	int nf = io_vFaces.size();
	int nv = io_vVertex.size();

	std::vector<char> dataBuffer;
	int fileLength = 4 + 4 + nv * 2 * sizeof(iv::vec3) + nf * 3 * sizeof(int);
	dataBuffer.resize(fileLength);

	char* ptr = &dataBuffer[0];

	memcpy(ptr,&nv,4);
	ptr += 4;
	memcpy(ptr,&nf,4);
	ptr += 4;

	unsigned int v3Size = sizeof(iv::vec3);

	for ( unsigned int i=0; i<nv; ++i )
	{
		iv::vec3 v = io_vVertex[i].mPositions;
		memcpy(ptr,&v,v3Size);
		ptr += v3Size;
	}

	for ( unsigned int i=0; i<nv; ++i )
	{
		iv::vec3 n = io_vVertex[i].mNormals;
		memcpy(ptr,&n,v3Size);
		ptr += v3Size;
	}

	for ( unsigned int j=0; j<nf; ++j )
	{
		int fi[3];
		for(int i = 0; i < 3; ++i)
			fi[i] = io_vFaces[j].mVerticesID[i];
		memcpy(ptr,fi,12);
		ptr += 12;
	}

	std::ofstream oFile(filePath,std::ios::binary);
	oFile.write(&dataBuffer[0],fileLength);
	oFile.close();

	std::string name(filePath);
	int iDot = name.find_first_of('.');
	std::string narFileName = name.substr(0,iDot) + ".nar";

	std::vector<char> narData;
	int narFileLength = 0;
	for(int i = 0; i < io_vNarrows.size(); ++i)
	{
		narFileLength += sizeof(iCathMesh::NarrowInfo);
		std::map<int,iv::vec3>& sb = io_vNarrows[i];
		narFileLength += sb.size() * (sizeof(iv::vec3) + sizeof(int));
	}
	narData.resize(narFileLength);
	ptr = &narData[0];

	// Narrow Extra Info.....................
	for(int i = 0; i < io_vNarrows.size(); ++i)
	{
		iCathMesh::NarrowInfo narrowInfo;
		narrowInfo.eValid = iCathMesh::NARROW_VALID;
		narrowInfo.iVertexCount = io_vNarrows[i].size();
		memcpy(ptr,&narrowInfo,sizeof(iCathMesh::NarrowInfo));
		ptr += sizeof(iCathMesh::NarrowInfo);
		for(std::map<int,iv::vec3>::iterator itr = io_vNarrows[i].begin(); itr != io_vNarrows[i].end(); ++itr)
		{
			memcpy(ptr,&itr->first,sizeof(int));
			ptr += sizeof(int);
			memcpy(ptr,&itr->second,sizeof(iv::vec3));
			ptr += sizeof(iv::vec3);
		}
	}

 	std::ofstream narFile(narFileName,std::ios::binary);
 	narFile.write(&narData[0],narFileLength);
 	narFile.close();

	printf("Save Succeeded!\n");
}

void CxExporter::SaveNarrowInfo(iCathMesh* pMesh,const char* filePath)
{

}
