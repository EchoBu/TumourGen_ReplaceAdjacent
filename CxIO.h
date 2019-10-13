#pragma once

#include "SiMath.h"
class iCathMesh;


class CxImporter
{
public:

	enum PostProcessing {
		IV_GenPerVertexNormal = 1,
		IV_GenPerFaceNormal = 2,
		IV_GenAdjacencyInfo = 4
	};

	static iCathMesh* LoadObjFile(const char* filePath, int postFlags = 0);

	static iCathMesh* LoadSTLFile(const char* filePath, int postFlags = 0);

	static iCathMesh* LoadVMBinFile(const char* filePath,int postFlags = 0);

	static iCathMesh* LoadVMBinFileEx(const char* filePath,int postFlags = 0);

	static iCathMesh* LoadSVMBinFile(const char* filePath);

	static iCathMesh* LoadNxVMBinFile(const char* filePath);

	static void loadNarFile(const char* filePath);

	static bool VerifySVMFile(const char *pData,int iBytes);
};




class CxExporter
{
public:
	// .svm file format:
	// MeshHeader : sizeof(MeshHeader).
	// All Vertex Position.
	// All Vertex Normal.
	// All Face Vertex Indices.
	// Tumor Info.
	// Opt. Tumor Face Indices.
	// Narrow Info.
	// Opt. Narrowing Part Vertex Healthy Info.
	static void SaveAsBinSVM(iCathMesh* pMesh,const char* filePath);

	static void SaveAs_VM_Ex(iCathMesh* pMesh,const char* filePath);
	static void SaveNarrowInfo(iCathMesh* pMesh,const char* filePath);
};