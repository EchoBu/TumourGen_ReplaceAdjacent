#include "stdafx.h"
#include "TextFileDataStream.h"


TextFileDataStream::TextFileDataStream() : mFileLength(0)
{

}

void TextFileDataStream::LoadDataFromFile(const char* filePath)
{
	//mFileStream.clear();

	mData.clear();

	mFileStream.open(filePath, std::ios::ate);

	if (!mFileStream.is_open())
	{
		printf("Failed to open file!\n");
		exit(1);
	}

	mFileLength = mFileStream.tellg();

	mData.resize(mFileLength);

	mFileStream.seekg(0, std::ios::beg);


	mFileStream.read(&mData[0], mFileLength);

	mFileStream.close();
}
