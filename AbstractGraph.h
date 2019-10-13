#pragma once
#include <vector>
class AbstractGraph
{
public:
	virtual float GetWeight(int p0Index, int p1Index) = 0;
	virtual float GetEvaDistance(int p0Index, int p1Index) = 0;
	virtual std::vector<int>& GetNeighbourList(int pindex) = 0;
	virtual int GetNodeCount() = 0;
};