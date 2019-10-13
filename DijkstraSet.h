#pragma once

class DijkstraSet
{
public:
	virtual void Add(int pindex) = 0;// required operations, add new pindex into set
	virtual  int ExtractMin() = 0;// required operations, remove and return the pindex which has the minimum distance
	virtual  bool IsEmpty() = 0;// required operations, return true if the element count in set==0
	virtual void DecreaseKey(int pindex) = 0;// necessary if implemented as a heap, when update shorter distance :dist[v]=dist[u] + dist_between(u, v) ;
};
