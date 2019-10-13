#pragma once

#include "MyPoint.h"
#include <list>
#include <vector>
#include <memory>
#include <set>
#include <iostream>
#include <Eigen\Dense>

using namespace Eigen;

class SimplifyObject
{
public:
	static bool recalculate;//�Ƿ������Ȩ
	static bool keepBorder;//�Ƿ��ϸ񱣳ֱ߽�

	struct Vertex;
	struct Edge;
	struct Face;

	//��������
	struct ArgMatrix
	{
		double a[4][4];

		ArgMatrix& operator+=(const ArgMatrix &b)
		{
			for (int i = 0;i < 4;i++) {
				for (int j = 0;j < 4;j++) {
					a[i][j] += b.a[i][j];
				}
			}
			return *this;
		}
		ArgMatrix& operator-=(const ArgMatrix &b)
		{
			for (int i = 0;i < 4;i++) {
				for (int j = 0;j < 4;j++) {
					a[i][j] -= b.a[i][j];
				}
			}
			return *this;
		}
		void clear()
		{
			for (int i = 0;i < 4;i++) {
				for (int j = 0;j < 4;j++) {
					a[i][j] = 0;
				}
			}
		}
	};

	//����ṹ��
	struct Vertex
	{
		MyPoint p;	//����λ��
		int id;	//������
		
		std::list<Face*> faceList;	//���
		std::list<Edge*> edgeList;	//�߱�

		ArgMatrix k;//�õ��������
		Vertex *pa;	//���鼯����

		Vertex* getroot();	//���鼯Ѱ��
		void join(Vertex *b);	//���鼯�ϲ� ���Լ�����b �ϲ���߱�
		Edge* findNeighborEdge(Vertex* b) const;	//�����ڵ��ұ�
		//void uniqueEdge(); //����ָ�� ȥ���ر�/��Ч��
	};

	//�߽ṹ��
	struct Edge
	{
		Vertex *ap, *bp;//���˶���

		MyPoint pos;//�����λ��
		double cost;//����������

		Vertex* findOtherVertex(const Vertex* p) const;//��֪һ������һ��
		bool update();	//ͨ�����鼯�������˵ĵ㣬��Ч����false
		void makeArg();	//ͨ�����˵ĵ��������λ��
		bool ifBorderEdge() const;	//����ñ��Ƿ�λ�ڱ߽�
	};

	//��ṹ��
	struct Face
	{
		ArgMatrix k;	//ϵ������
		Vertex *ap, *bp, *cp;	//������λ��

		void makeArg();	//����ϵ������
		bool update(); //ͨ�����鼯���¶��㣬��Ч����false
		void increaseNeighborPointArg();	//���Լ�ϵ���ӵ�������
		void decreaseNeighborPointArg();	//���������Լ�ϵ���۳�
	};

private:
	std::vector<std::unique_ptr<Vertex>> vertexPool;//����
	std::vector<std::unique_ptr<Edge>> edgePool;//�߳�
	std::vector<std::unique_ptr<Face>> facePool;//���

	
public:
	

	bool Load(MatrixXd V, MatrixXi F);

	void simpify(double factor);//���м򻯣��򻯱�Ϊfactor

	void Save(MatrixXd& OV, MatrixXi& OF);
};