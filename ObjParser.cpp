#include "stdafx.h"
#include "ObjParser.h"
#include <assert.h>
#include <cstdlib>
#include <map>
#include <set>
#include "aMesh.h"

ObjParser::ObjParser(const char* cStrFilePath, iCathMesh* o_pMesh) : m_pMesh(o_pMesh)
{
	LoadFile(cStrFilePath);
}


ObjParser::~ObjParser()
{
}


void ObjParser::LoadFile(const char* filePath)
{
	mFileStream.LoadDataFromFile(filePath);

	mStart = mFileStream.mData.begin();
	mCurrent = mStart;
	mEnd = mFileStream.mData.end();

	Parse();
}


bool ObjParser::IsSeperator(char c)
{
	return (c == ' ') || (c == '\t') || (c == '\n');
}

void ObjParser::SkipCurrentLine()
{
	while (mCurrent != mEnd && *mCurrent != '\n')
		++mCurrent;
	if (mCurrent == mEnd)
		return;
	++mCurrent;
}

std::string ObjParser::GetNextToken()
{
	assert(IsSeperator(*mCurrent));

	while (mCurrent != mEnd && IsSeperator(*mCurrent))
		++mCurrent;

	if (mCurrent == mEnd)
		return std::string("");

	DataPtr p = mCurrent;

	while (mCurrent != mEnd && !IsSeperator(*mCurrent))
		++mCurrent;
	return std::string(p, mCurrent);
}

void ObjParser::Parse()
{
	if (mCurrent == mEnd)
		return;
	if (m_pMesh == 0)
		return;
	while (mCurrent != mEnd)
	{
		switch (*mCurrent)
		{
		case '\n':	// Enter
		case '#':	// Comments
		{
			SkipCurrentLine();
			break;
		}
		case 'm':	// mtllilb
		{
			std::string s(mCurrent, mCurrent + 6);
			if (s != std::string("mtllib"))
			{
				printf("Can't parse the token: %s\n", s.c_str());
				SkipCurrentLine();
			}
			mCurrent += 6;
			GetNextToken();

			break;
		}
		case 'v':
		{
			++mCurrent;
			if (*mCurrent == ' ')  // Vertex Positon Info.
			{
				std::string s;
				float x, y, z;

				s = GetNextToken();		x = (float)std::atof(s.c_str());
				s = GetNextToken();		y = (float)std::atof(s.c_str());
				s = GetNextToken();		z = (float)std::atof(s.c_str());

				iCathMesh::Vertex v;
				v.mPositions = iv::vec3(x, y, z);
				v.mID = m_pMesh->mVertexMap.size();
				m_pMesh->mVertexMap[v.mID] = v;
			}
			else if (*mCurrent == 't') // Vertex TexCoords Info.
			{
				++mCurrent;

				std::string s;
				float u, v, w;

				s = GetNextToken();		u = (float)std::atof(s.c_str());
				s = GetNextToken();		v = (float)std::atof(s.c_str());
				s = GetNextToken();		w = (float)std::atof(s.c_str());

				// We don't consider texture coordinate right now!
			}
			else if (*mCurrent == 'n')	// Vertex Normal Info.
			{
				++mCurrent;
				std::string s;
				float x, y, z;

				s = GetNextToken();		x = (float)std::atof(s.c_str());
				s = GetNextToken();		y = (float)std::atof(s.c_str());
				s = GetNextToken();		z = (float)std::atof(s.c_str());

				// We neglect normal for now!
			}
			else
			{
				printf("Can't parse this token!\n");
				SkipCurrentLine();
			}
			break;
		}
		case 'f':	// Face Index
		{
			mCurrent++;

			iCathMesh::Face f;

			for (int i = 0; i < 3; ++i)
			{
				std::string s = GetNextToken();
				int slash1 = s.find_first_of('/');
				int slash2 = s.find_last_of('/');

				unsigned int vID = 0;
				unsigned int texID = 0;
				unsigned int nID = 0;

				// If there's only vertex indices
				if (slash1 < 0)
				{
					vID = std::atoi(s.c_str());
				}
				else if (slash1 == slash2) // No Normal Indices;
				{
					std::string vStr = s.substr(0, slash1);
					std::string tStr = s.substr(slash1 + 1, s.length() - slash1 - 1);
					vID = std::atoi(vStr.c_str());
					texID = std::atoi(tStr.c_str());
				}
				else if ((slash2 - slash1) == 1)	// x//x  No texCoord Indice
				{
					std::string vStr = s.substr(0, slash1);
					std::string nStr = s.substr(slash2 + 1, s.length() - slash2 - 1);
					vID = std::atoi(vStr.c_str());
					nID = std::atoi(nStr.c_str());
				}
				else
				{
					std::string vStr = s.substr(0, slash1);
					std::string tStr = s.substr(slash1 + 1, slash2 - slash1 - 1);
					std::string nStr = s.substr(slash2 + 1, s.length() - slash2 - 1);
					vID = std::atoi(vStr.c_str());
					texID = std::atoi(tStr.c_str());
					nID = std::atoi(nStr.c_str());
				}
				if (vID == 0)
					printf("Alert! Obj File Index Start From 1!\n");
				else
					f.mVerticesID.push_back(vID - 1);
				// We neglect texture coords and normals for .obj files for now!
			}
			f.mID = m_pMesh->mFaceMap.size();
			m_pMesh->mFaceMap[f.mID] = f;

			break;
		}

		case 'g':  // Group Info.
		{
			mCurrent++;
			std::string meshName = GetNextToken();

			// We neglect group information for .obj file for now!

			break;
		}
		case 'u':	// usemtl
		{
			SkipCurrentLine();
			break;
		}
		default:
			mCurrent++;
			break;
		}
	}
}

