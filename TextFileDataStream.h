#pragma once

#include <fstream>
#include <vector>
#include <string>


class TextFileDataStream
{
	typedef std::vector<char> DataStream;
	typedef std::vector<char>::iterator DataPtr;


public:
	std::ifstream	mFileStream;

	DataStream		mData;

	unsigned int	mFileLength;

public:
	TextFileDataStream();

	void LoadDataFromFile(const char* filePath);
};