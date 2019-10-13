#include "stdafx.h"
#include "SimplifyObject.h"

#pragma warning(disable:4996)

bool SimplifyObject::recalculate = DefaultRecalculate;
bool SimplifyObject::keepBorder = DefaultKeepBorder;



void SimplifyObject::Edge::makeArg()
{
	//����Ǳ߽�ߣ�������Ҫ�ϸ񱣳ֱ߽�
	if (keepBorder && ifBorderEdge()) {
		cost = INFINITY;
		return;
	}

	//���㲿��

	//������Ĵ��۾������
	ArgMatrix k = ap->k;
	k += bp->k;

	//����ʽ����
	auto det = [](
		double a, double b, double c,
		double d, double e, double f,
		double g, double h, double i)
	{
		return a*e*i + d*h*c + b*f*g - a*f*h - b*d*i - c*e*g;
	};

	//�������˷� pos * k * posT
	auto calCost = [this](const MyPoint &pos, const ArgMatrix &k) {
		double cost = k.a[3][3];
		for (int i = 0;i < 3;i++) {
			for (int j = 0;j < 3;j++) {
				cost += pos.p[i] * pos.p[j] * k.a[i][j];
			}
			cost += pos.p[i] * k.a[3][i] * 2;
		}
		return cost;
	};

	//����ķ�ⷽ��
	double D = det(
		k.a[0][0], k.a[0][1], k.a[0][2],
		k.a[1][0], k.a[1][1], k.a[1][2],
		k.a[2][0], k.a[2][1], k.a[2][2]
	);
	if (abs(D) < eps) {//���ϵ�����󲻿���
		pos = (ap->p + bp->p) / 2;
		cost = calCost(pos, k);
		//����Ǳ߽磬���ӵ��ƶ��Ĵ���
		if (this->ifBorderEdge()) cost += norm(ap->p - pos) + norm(bp->p - pos);
		return;
	}

	double x = det(
		-k.a[0][3], k.a[0][1], k.a[0][2],
		-k.a[1][3], k.a[1][1], k.a[1][2],
		-k.a[2][3], k.a[2][1], k.a[2][2]
	) / D;
	double y = det(
		k.a[0][0], -k.a[0][3], k.a[0][2],
		k.a[1][0], -k.a[1][3], k.a[1][2],
		k.a[2][0], -k.a[2][3], k.a[2][2]
	) / D;
	double z = det(
		k.a[0][0], k.a[0][1], -k.a[0][3],
		k.a[1][0], k.a[1][1], -k.a[1][3],
		k.a[2][0], k.a[2][1], -k.a[2][3]
	) / D;

	//��һ�ֽⷨ
	/*Matrix4d A;
	A << k.a[0][0] ,k.a[0][1],k.a[0][2],k.a[0][3],
		k.a[0][1] ,k.a[1][1],k.a[1][2],k.a[1][3], 
		k.a[0][2] ,k.a[1][2],k.a[2][2],k.a[2][3], 
		0,0,0,1;
	Vector4d B;
	B << 0, 0, 0, 1;
	Vector4d C = A.inverse() * B;

	pos = { C(0),C(1) ,C(2) };*/

	pos = { x, y, z };
	cost = calCost(pos, k);
	//����Ǳ߽磬���ӵ��ƶ��Ĵ���
	if (this->ifBorderEdge()) cost += norm(ap->p - pos) + norm(bp->p - pos);
}

//�ж��Ƿ�Ϊ�߽�ߣ�ͨ���ܱ���ͱߵ������жϣ�
bool SimplifyObject::Edge::ifBorderEdge() const
{
	return ap->edgeList.size() != ap->faceList.size() || bp->edgeList.size() != bp->faceList.size();
}

//bool SimplifyObject::Load(const char *filename)


//�ѱȽϺ���
static bool edgeComp(const SimplifyObject::Edge* a, const SimplifyObject::Edge *b)
{
	assert(!isnan(a->cost) && !isnan(b->cost));
	if (a->cost == b->cost) {
		return a < b;
	}
	else {
		return a->cost < b->cost;
	}
}

//��������
void SimplifyObject::simpify(double factor)
{
	int nowFaceNum = facePool.size();
	int finalFaceNum = (int)(nowFaceNum * factor);

	//finalFaceNum = nowFaceNum - 10;

	
	//��set�����
	std::set<Edge*, std::function<bool(const Edge*, const Edge*)>> ss(edgeComp);

	//����
	for (auto &x : edgePool){
		x->makeArg();
		ss.insert(x.get());
	}

	//������ ��ֹ����
	while (nowFaceNum > finalFaceNum)
	{
		
		//ȥ��������С��
		Edge* e = *ss.begin();
		ss.erase(e);

		assert(!isinf(e->cost) && !isnan(e->cost));

		auto* pa = e->ap;
		auto* pb = e->bp;
		//����ɾ�������pa��Ϊpa��root��������������Ϣ�Ѿ����ڣ�
		if(pa != pa->getroot() || pb != pb->getroot() || pa == pb) continue;

		//ϵ���������
		pa->k += pb->k;

		//�ϲ� pb����pa
		pb->join(pa);

		//�޸ĺϲ����λ��
		pa->p = e->pos;

		//ȥ�ر�\��Ч��
		//ע������update���ı߶���Ҫɾ��
		for (auto &x = pa->edgeList.begin(); x != pa->edgeList.end(); ) {
			ss.erase(*x);
			if (!(*x)->update()) { //��Ч��
				x = pa->edgeList.erase(x);
				continue;
			}
			x++;
		}
		//�Ա߱����һ��ȥ��
		pa->edgeList.sort([&pa](const Edge* a, const Edge* b) {
			Vertex* ta = a->findOtherVertex(pa);
			Vertex* tb = b->findOtherVertex(pa);
			if (ta == tb) {
				return a < b;
			} else {
				return ta < tb;
			}
		});
		pa->edgeList.unique([&pa](const Edge* a, const Edge* b) {
			return a->findOtherVertex(pa) == b->findOtherVertex(pa);
		});


		//��¼�޸Ĺ��ı�
		std::vector<Edge*> editEdgeList;
		//��������ؼ��㣬�޸ĵı�ֻ��pa�ܱߵı�
		if (!recalculate) {
			editEdgeList.insert(editEdgeList.end(), pa->edgeList.begin(), pa->edgeList.end());
		}
		
		//���¼���Ȩֵ�����������Ϣ
		//ɾȥ�ϲ�������
		pa->faceList.sort();
		pa->faceList.unique();
		for (auto &nFace = pa->faceList.begin(); nFace != pa->faceList.end(); nFace++) {
			if (!(*nFace)->ap) continue;
			(*nFace)->update();
			
			//�����Ҫ���¼��㣬��Ҫ�޸��ڵ����еı�
			if (recalculate) {
				(*nFace)->decreaseNeighborPointArg(); //����Χ���ȥ֮ǰ��arg
				editEdgeList.insert(editEdgeList.end(), (*nFace)->ap->edgeList.begin(), (*nFace)->ap->edgeList.end());
				editEdgeList.insert(editEdgeList.end(), (*nFace)->bp->edgeList.begin(), (*nFace)->bp->edgeList.end());
				editEdgeList.insert(editEdgeList.end(), (*nFace)->cp->edgeList.begin(), (*nFace)->cp->edgeList.end());
			}
		}

		//�������ɵĵ���Χ���棬ȥ����Ч��
		//�������Ȩֵ
		//�ٸ��������ڵ�Ȩֵ
		for (auto &nFace = pa->faceList.begin(); nFace != pa->faceList.end(); ) {
			if (!(*nFace)->ap || !(*nFace)->update()) { //��Ч��
				if ((*nFace)->ap) {
					(*nFace)->ap = nullptr;//�����ʧЧ
					nowFaceNum--;//�������ټ���
				}
				nFace = pa->faceList.erase(nFace);
				continue;
			}

			if (recalculate) {
				(*nFace)->makeArg(); //����Ȩֵ
				(*nFace)->increaseNeighborPointArg(); //����Χ��������ڵ�arg
			}

			nFace++;
		}

		//�ٸ��µ����ڱߵ�Ȩֵ
		//Ϊ�˼��ٶԶѵĲ����������޸Ľ���ȥ�ش���
		sort(editEdgeList.begin(), editEdgeList.end());
		editEdgeList.resize((unique(editEdgeList.begin(), editEdgeList.end()) - editEdgeList.begin()));
		for (auto &x : editEdgeList){
			ss.erase(x);
			x->update();
			x->makeArg();
			ss.insert(x);
		}

	}

	//��������±�Ų������ṹ
	std::vector<std::unique_ptr<Vertex>> _vertexPool;
	std::vector<std::unique_ptr<Edge>> _edgePool;
	std::vector<std::unique_ptr<Face>> _facePool;
	for (auto &x : vertexPool) {
		if (x->getroot() == x.get()) {
			x->id = _vertexPool.size() + 1;
			_vertexPool.push_back(std::move(x));
		}
	}
	for (auto &x : edgePool) {
		if (x->update()) {
			_edgePool.push_back(std::move(x));
		}
	}
	for (auto &x : facePool) {
		if (x->ap && x->update()) {
			_facePool.push_back(std::move(x));
		}
	}
	vertexPool = std::move(_vertexPool);
	edgePool = std::move(_edgePool);
	facePool = std::move(_facePool);
}


//void SimplifyObject::Save(const char * filename)


void SimplifyObject::Save(MatrixXd& OV, MatrixXi& OF)
{
	OV.resize(vertexPool.size(), 3);
	OF.resize(facePool.size(), 3);

	int count = 0;
	for (auto &x : vertexPool) {
		OV(count, 0) = x->p.x;
		OV(count, 1) = x->p.y;
		OV(count, 2) = x->p.z;
		count++;		
	}

	count = 0;
	for (auto &x : facePool) {
		
		OF(count, 0) = x->ap->id-1;
		OF(count, 1) = x->bp->id-1;
		OF(count, 2) = x->cp->id-1;
		count++;
	}
}

void SimplifyObject::Face::makeArg()
{
	MyPoint n = cross(ap->p - cp->p, bp->p - cp->p);
	n /= abs(n);

	double tmp[] = { n.x, n.y, n.z, -dot(n, ap->p) };
	for (int i = 0;i < 4;i++) {
		for (int j = 0;j < 4;j++) {
			k.a[i][j] = tmp[i] * tmp[j];
		}
	}
}

bool SimplifyObject::Face::update()
{
	ap = ap->getroot();
	bp = bp->getroot();
	cp = cp->getroot();
	return !(ap == bp || bp == cp || ap == cp);
}

void SimplifyObject::Face::increaseNeighborPointArg()
{
	ap->k += k;
	bp->k += k;
	cp->k += k;
}

void SimplifyObject::Face::decreaseNeighborPointArg()
{
	ap->k -= k;
	bp->k -= k;
	cp->k -= k;
}

SimplifyObject::Vertex* SimplifyObject::Vertex::getroot()
{
	if (!pa) return this;
	return pa = pa->getroot();
}

void SimplifyObject::Vertex::join(Vertex * b)
{
	pa = b;
	b->faceList.insert(b->faceList.end(), faceList.begin(), faceList.end());
	faceList.clear();
	b->edgeList.insert(b->edgeList.end(), edgeList.begin(), edgeList.end());
	edgeList.clear();
}

SimplifyObject::Edge * SimplifyObject::Vertex::findNeighborEdge(Vertex * b) const
{
	for (auto &x : edgeList) {
		if (x->findOtherVertex(this) == b) return x;
	}
	return nullptr;
}
	

SimplifyObject::Vertex* SimplifyObject::Edge::findOtherVertex(const Vertex * p) const
{
	assert(ap == p || bp == p);
	return (Vertex*)((int)ap ^ (int)bp ^ (int)p);
}

bool SimplifyObject::Edge::update()
{
	ap = ap->getroot();
	bp = bp->getroot();
	return ap != bp;
}

bool SimplifyObject::Load(MatrixXd V, MatrixXi F)
{
	for (int i = 0 ; i < V.rows(); i++)
	{
		vertexPool.emplace_back(new Vertex);
		auto& vP = *vertexPool.back();
		vP.id = (int)vertexPool.size();
		vP.k.clear();
		vP.pa = nullptr;
		vP.p.x = V(i, 0);
		vP.p.y = V(i, 1);
		vP.p.z = V(i, 2);
		
	}

	for (int i = 0 ; i < F.rows(); i++)
	{
		int ai = F(i, 0);
		int bi = F(i, 1);
		int ci = F(i, 2);
		auto &a = *vertexPool[ai], &b = *vertexPool[bi], &c = *vertexPool[ci];
		facePool.emplace_back(new Face);
		auto& f = *facePool.back();

		//����߱�
		Edge *ae, *be, *ce;
		ae = a.findNeighborEdge(&b);
		be = b.findNeighborEdge(&c);
		ce = c.findNeighborEdge(&a);
		if (!ae) {
			edgePool.emplace_back(new Edge);
			ae = edgePool.back().get();
			a.edgeList.push_back(ae);
			b.edgeList.push_back(ae);
		}
		if (!be) {
			edgePool.emplace_back(new Edge);
			be = edgePool.back().get();
			b.edgeList.push_back(be);
			c.edgeList.push_back(be);
		}
		if (!ce) {
			edgePool.emplace_back(new Edge);
			ce = edgePool.back().get();
			c.edgeList.push_back(ce);
			a.edgeList.push_back(ce);
		}

		//������ߵ��ϵ
		f.ap = &a, f.bp = &b, f.cp = &c;
		ae->ap = &a, be->ap = &b, ce->ap = &c;
		ae->bp = &b, be->bp = &c, ce->bp = &a;
		a.faceList.push_back(&f);
		c.faceList.push_back(&f);
		b.faceList.push_back(&f);
		f.makeArg();//������ϵ������
		f.increaseNeighborPointArg();//���¶˵�ϵ��ֵ
	}
	return true;
}