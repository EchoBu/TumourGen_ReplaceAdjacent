#pragma once

#include <vector>
#include <string>
#include "aMesh.h"
#include "TextFileDataStream.h"

class CxMesh;

class ObjParser
{
	typedef std::vector<char>::iterator DataPtr;

public:
	TextFileDataStream mFileStream;

	DataPtr mStart;
	DataPtr mCurrent;
	DataPtr mEnd;


	iCathMesh *m_pMesh;

public:
	ObjParser(const char* cStrFilePath, iCathMesh* o_pMesh);
	~ObjParser();

	void			LoadFile(const char* filePath);

	void			Parse();

	bool			IsSeperator(char c);

	// Skip Current Line.
	// After calling, the DataPtr points the very first of the next line, Or reaches the End of the File.
	void			SkipCurrentLine();

	// Assume mCurrent is pointing at seperators.
	std::string		GetNextToken();

	void			PostProcess();

};
