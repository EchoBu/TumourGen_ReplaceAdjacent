#pragma once

#include "SiMath.h"
#include <map>
#include <set>
#include <memory>
#include <vector>
#include <Eigen\Dense>
#include <Eigen\Sparse>
#include "solver.h"
#include "arapsolver.h"


#define USE_LEGACY 0

class CxCollisionTest;
class CxExporter;
class CxPoints;
class CxLines;

struct iCathPoint {
	int mID;
	iv::vec3 mHealthPosition;
	iv::vec3 mAbnormalPosition;
	iCathPoint():mID(-1),
		mHealthPosition(0.0f),
		mAbnormalPosition(0.0f)
	{

	}
};

class BoundingBox
{
public:
	BoundingBox() : vCenter(0.0f),vExtent(1.0f) {}

	iv::vec3 vCenter;
	iv::vec3 vExtent;
};

class iCathMesh
{
public:
	typedef std::vector<int> IntArray;

	enum Validation {
		TUMOR_VALID = 0x1,
		NARROW_VALID = 0x2
	};

	struct MeshHeader
	{
		int iTumorCount;
		int iNarrowCount;
		int iFaceCount;
		int iVertexCount;
	};

	struct TumorInfo
	{
		Validation	eValid;
		int			iFaceCount;
		float		fRadius;
		iv::vec3	vCenter;
	};

	struct NarrowInfo
	{
		Validation	eValid;
		int			iVertexCount;
	};


	struct AuxVertex
	{
		int iID;
		iv::vec3 vPosition;

		AuxVertex(int id,const iv::vec3& vp) : iID(id),vPosition(vp) {}
	};

	struct Edge
	{
		int iV1;
		int iV2;

		Edge(int _iV1,int _iV2)
		{
			iV1 = _iV1 < _iV2 ? _iV1 : _iV2;
			iV2 = _iV1 < _iV2 ? _iV2 : _iV1;
		}

		bool operator < (const Edge& e) const
		{
			if(iV1 < e.iV1)
				return true;
			else if( iV1 == e.iV1 && iV2 < e.iV2)
				return true;
			else
				 return false;
		}

		bool operator == (const Edge& e) const
		{
			return (iV1 == e.iV1) && (iV2 == e.iV2);
		}

		bool containVertex(int vid) const
		{
			return vid == iV1 || vid == iV2;
		}
	};

	struct Face
	{
		iv::vec3	mNormal;
		IntArray	mVerticesID;
		IntArray	mNeighborFaceID;
		iv::vec3	mGravity;
		int			mID;

		

		bool ContainVertexID(int vID) const
		{
			bool ret = false;
			for( size_t i = 0 ; i < mVerticesID.size() ; i++ )
				if( mVerticesID[i] == vID)
					ret = true;
			return ret;
		}

		int GetExclusiveVertexIDByEdge(const Edge& rEdge) const
		{
			int retVal = -1;
			int iV1 = rEdge.iV1;
			int iV2 = rEdge.iV2;
			for(int i = 0; i < mVerticesID.size(); ++i)
			{
				if(mVerticesID[i]!=iV1 && mVerticesID[i]!=iV2)
					retVal = mVerticesID[i];
			}
			return retVal;
		}
	};

	// "MidVertex" Means The Middle Point Of An Edge.
	// For The Purpose Of Split Face.
	struct MidVertex
	{
		Edge	edge;
		int		iVertexID;
		MidVertex(const Edge& rEdge,int vid) : edge(rEdge),iVertexID(vid) {}
	};

	// "AuxFace" Is A Face To Be Splitted By Points On Its Edges.
	struct AuxFace
	{
		int iFaceID;
		std::vector<MidVertex> vNewVertices;
		AuxFace(int faceID = -1) : iFaceID(faceID)
		{
			vNewVertices.clear();
		}
	};

	struct Vertex
	{
		IntArray mNeighborVertexID;
		IntArray mNeighborFaceID;
		iv::vec3 mPositions;
		iv::vec3 mNormals;
		
		int		 mID;


		bool operator < (const Vertex& v) const
		{
			return mPositions < v.mPositions;
		}
	};


	typedef std::map<int,Face>				FaceMap;
	typedef std::map<int,Vertex>			VertexMap;
	typedef std::map<Edge,std::vector<int>> EdgeMap;
	
	typedef std::set<int>					FaceSet;
	typedef std::vector<FaceSet>			TumorArray;
	typedef std::map<int,iv::vec3>			NarrowMap;
	typedef std::vector<NarrowMap>			NarrowArray;

public:
	
	iCathMesh();
	~iCathMesh();

	int GetErrorFaceID() const;

	void SaveAsSTL(const char* pStrFilePath);

	void saveAsObj(const char* pStrFilePath);

	CxLines* getEdgeLines() { return m_EdgeLines.get(); }

	CxLines* getActiveEdgeLines() { return m_ActiveEdgeLines.get(); }

	CxLines* getSelectedFace() { return m_SelectedFace.get(); }

	void removeSelectedFace();

	void reverseFacesAndSaveAsObj();


	// 进行预计算
	void doArapmodelingPrecompute();
	void doArapmodelingPrecompute_TumorMode();

	//进行ARAP形变
	//@ offset 偏移量，决定变形强度
	//@ flag 标志，0-构造狭窄，1-构造血管瘤
	void doDeform(const iv::vec3& offset, int flag);	

	// 高亮显示faces_id中的三角形
	void drawTriangles(IntArray faces_id);	

	// 得到faces_id中的顶点ID，存放在vertices_id中
	void getVIDByFID(const IntArray faces_id, IntArray& vertices_id);

	// 得到由vertices_id中的顶点组成的顶点矩阵，结果存在vertex_matrix中
	// vertex_matrix中每一行存储着一个顶点的x,y,z坐标	
	void getVexticesMatrix(Eigen::MatrixXd& vertex_matrix, const IntArray vertices_id);     	

	// 得到由faces_id中的顶点组成的顶点矩阵，结果存在face_matrix中
	// face_matrix中每一行存储着一个面的三个当点的ID	
	void getFaceMatrix(Eigen::MatrixXi& face_matrix, const IntArray faces_id);
		
	// 得到固定点（1最外围的不在变形影响范围内的点，2控制点）在'@vertex_ids_all'中的的下标集合
	//@vertex_ids_all       所有顶点ID的集合
	//@face_ids_free        自由顶点ID的集合
	//@face_ids_control     控制点ID的集合
	//@fixed_vertices_index 储存结果
	void getFixedVerticesIdVextor(const IntArray vertex_ids_all, 
		const IntArray face_ids_free, 
		const IntArray face_ids_control,
		Eigen::VectorXi& fixed_vertices_index);
	
	//得到固定点的坐标信息，结果存储在fixed_vertices_coordinate中
	void getVerticesCoordinateById(const Eigen::VectorXi fixed_vertices_index,
		Eigen::MatrixXd& fixed_vertices_coordinate);

	// 得到faceID_B中的所有顶点在集合vertexID_A中的下标
	// 结果保存在BindexInA中
	void getIndexes(const Eigen::VectorXi vertexID_A, 
		const IntArray faceID_B, IntArray& BindexInA);

	// 设置控制点的固定位置约束
	// @face_ids_control             控制点所在的面的ID
	// @indexes		                 控制点 在 固定点 中的下标 
	// @offset		                 偏移量
	// @fixed_vertices_coordinate    储存结果（控制点的目标坐标）
	void setContrlVerticesCoordinate(const IntArray face_ids_control, const IntArray indexes,
		const iv::vec3& offset, Eigen::MatrixXd& fixed_vertices_coordinate,const int flag);

	// 更新 mVerticesMap
	void updateVerticesMap(Eigen::MatrixXd& update_vertices,IntArray v);	

	// 得到初始网格
	void getInitMesh();
	// 得到上一步的网格
	void getBackupMesh();	

	// 清理face_ids_control，face_ids_free 和 face_ids_all
	void clearSelectedFace();	

	//在m_Tumorface中找到点p坐在的面，返回该面的ID
	int findCrossFaceId_InTumorFace(const iv::vec3& p);
	
	//判断p是否在面faceId中
	bool isPointInTriangle(iv::vec3 P, int faceId);

	//得到面face_id的重心，结果存在p中
	void getCenterOfGravity(int face_id, iv::vec3& p);

	//在m_Tumorface中寻找中心面周围的一定范围的面
	//@ face_id 中心面
	//@ thickness 范围大小
	//@ result 结果
	void getMoreFaceInTumorFace(int face_id, float thickness, IntArray& result);
	
	//在所有面中寻找点p周围的一定范围的面
	//@ p 中心点
	//@ thickness 范围大小
	//@ vCollideFace 结果
	void getMoreFace(const iv::vec3 &p, float thickness, IntArray &vCollideFace);

	//得到faces_id_now的2-邻域面，存储在faces_id_more中
	void getMoreNeighber(IntArray faces_id_now, IntArray& faces_id_more);

	//计算面的法向量
	void CalcPerTumorFaceNormal();
	void CalcPerFaceNormal(IntArray& faces_id);
	void CalcPerTumorFaceNormalForSim();

	//计算顶点的法向量	
	void CalcPerTumorVertexNormal();
	void CalcPerVertexNormal(IntArray& faces_id);
	void CalcPerTumorVertexNormalForSim();

	//计算血管的近似半径
	float getVesselRadius(const iv::vec3 &p1,float t);

	//在肿瘤表面进行不规则的形变
	void DrawRandomBending();

	//得到 [a,b)之间的随机数；
	int getRandom(int a, int b);

	//设置构造LittleTumor需要的控制点，自由点和需要考虑的全部点
	void SelectLittleTumorFaces(iv::vec3 &p);
	
	// 设置构造Tumor需要的控制点，自由点和需要考虑的全部点
	void SelectFaceForDeform_TumorMode(const iv::vec3& p);
	
	// 网格光顺
	// @faces_id    进行光顺面
	// @times      迭代次数
	void MeshSmothing(const IntArray faces_id,int times);
	// 网格光顺封装
	void MeshSmothing();

	// 网格简化
	void MeshSimplification(const IntArray faces_id);
	void MeshSimplification();

	// 网格优化
	// @faces_id    进行优化的面
	// @fixed_vertices_id    优化过程中的固定点
	void MeshOptimization(const IntArray faces_id,const IntArray fixed_vertices_id);
	// 网格优化封装
	void MeshOptimization();

	//当顶点i和顶点j构成边的时候返回true
	bool isEdge(int vertice_i, int vertice_j);

	//计算面face_id中三个角的余切值，并返回（cotA,cotB,cotC）
	Eigen::Vector3d ComputeCotangent(int face_id);

	//计算余切权值矩阵
	void computeCotWeight(Eigen::MatrixXf& weight_, IntArray face_ids, IntArray vertex_ids);

	//得到"verticesID"中的边界顶点，结果存在border_vertices_id
	//border_vertices_id中的元素是边界点在集合vertices_id中的下标
	void getBorderIndex(IntArray &border_vertices_id,  IntArray vertices_id);

	 
	//返回在集合vertices_id中点vertex_index的邻点数量
	int getNeighborVerticesNumber(IntArray vertices_id,int vertex_index);

	//对faces_id_large和faces_id_small做差，得到结果faces_id_result
	void get_difference(IntArray faces_id_large, IntArray 
		faces_id_small, IntArray& faces_id_result);
	
	
	
	
	arap::demo::Solver* solver = nullptr;// arap solver	
	Eigen::MatrixXd fixed_vertices_coordinate;//固定点坐标	
	IntArray face_ids_control;//其上顶点为控制点
	IntArray face_ids_free;//其上顶点为自由点

	//所有相关点，在进行运算时只需考虑这其中的点，不需要计算整个网格
	IntArray face_ids_all, vertex_ids_all;
	
	IntArray index_in_fixed_vertices_id;//控制点在固定点中的下标	

	float sum_b;//单次变形中偏移量的总和
	
	IntArray m_TumorFace;//肿瘤上的面
	IntArray m_TumorVertex;//肿瘤上的点
	std::vector<IntArray>m_TumorFace_Part;
	float mVesselRadius;//血管近似半径
	float mLengthBetweenTwoVPick;//选narrow点的时候两个点之间的距离	
	

	// Generate Adjacenty Information.
#if USE_LEGACY
	void GenAdjacency(void);
#else
	void GenAdjacency(void);
#endif
	void CalcPerFaceNormal();
	void CalcPerVertexNormal();
	void CalcPerFaceGravity();//add

	void LoadUpToGpu(void);

	void AllocateGpuBuffer();

	iv::mat4 getModelMatrix() { return m_ModelMatrix; }
	void setModelMatrix(const iv::mat4& modelMat) { m_ModelMatrix = modelMat; }

	BoundingBox getBoundingBox();

	void SetDirty();

	void RefineVertexPosition();

	void setActiveEdgeIndex(int index);

	void fillActiveHole();

	void removeDuplicates();

	void removeThisEdgeAndNeighborFaces();

	void reverseSelectedFace();

	std::shared_ptr<iCathMesh> CreateSubMeshBySelectFace(int faceId);

	void Render();

	void OptimizedWayToGrowTumor(const iv::vec3& vCenter,float fRadius);

	std::shared_ptr<iCathMesh> SeperateTumor(const iv::vec3& vCenter,float fRadius);
	
	std::shared_ptr<iCathMesh> CreateSubMesh(const std::vector<int>& vSubset);

	void GetTumorSubMeshes(std::vector<std::shared_ptr<iCathMesh> >& vTumorMeshes, bool bFillHole = false);

	void GetNarrowPoints(std::vector<std::shared_ptr<CxPoints> >& vNarrowPoints);

	void NarrowMesh(const iv::vec3& vPick1,const iv::vec3& vPick2,std::shared_ptr<iCathMesh> *o_pNarrowedMesh = nullptr);
	
	void NarrowMesh(int startId,int endId,std::shared_ptr<iCathMesh> *o_pNarrowedMesh = nullptr);

	std::vector<int> vErrorID;

	FaceMap						mFaceMap;
	VertexMap					mVertexMap;
	std::vector<FaceMap>		mFaceMapBackup;
	std::vector<VertexMap>		mVertexMapBackup;
	FaceMap						mFaceMapInit;
	VertexMap					mVertexMapInit;

	EdgeMap		mEdges;
	TumorArray	mTumors;
	NarrowArray	mNarrows;
	std::vector<TumorInfo> mTumorInfos;


	float m_fRecentTumorRadius;
	iv::vec3 m_vRecentTumorCenter;
private:

	void _createEdgeLines();

	// Generate Adjacency Information;
	void GenEdgeAdjacency();
	void GenFaceAdjacency();
	void GenVertexAdjacency();
	void InValidAdjacencyInfo();

	std::set<int> FindAllConnectFaces(int seedId);

	bool CheckAdjacentyComplete();
	
	void FillHole();

	void _reverseFaceWinding(int index);

	void GetNarrowedFaces(int iStart,int iEnd,std::vector<std::vector<int> >& o_vNarrowedFace);

	// Check whether the mesh has 1 and only 1 hole.
	// A crack does not count for a hole.
	bool IsOnlyOneHole(std::vector<int>* pVertices = 0);

	std::shared_ptr<iCathMesh> SeperateBumpMesh(int iAnyFaceOnTumor);

	void GetFaceVerticesByIndex(int i_iIndex, iv::vec3& o_vP1,iv::vec3& o_vP2,iv::vec3& o_vP3);

	// Collision Detection Related Code. Put Here Temporary.
	// Return Code : 8 bit Only 3 Valid. The Lower 3 bits valids.
	//				 0 means the vertex is outside the sphere while 1 means the other way.
	char SphereVsTriangle(const iv::vec3 &vCenter,float fRadius,const iv::vec3 &v1,const iv::vec3 &v2,const iv::vec3 &v3);


	// Do Intersection Test Between Sphere(@vCenter,@fRadius) With Triangles(@vTriangles).
	// Return True If All Edges Intersect With Sphere At Most 1 Vertex. And mEdge2Vertex Stores The Results.
	// Return False If There Are More Than 1 Intersection Points And Data In @mEdge2Vertex Are Invalid.
	bool CheckAllEdgeWithOneIntersection(const iv::vec3& vCenter,float fRadius,const std::vector<int>& vTriangles,std::map<Edge,Vertex>& mEdge2Vertex);

	// Get The Next New Vertex ID.
	int GenNextNewVertexID();
	int GetNextNewFaceID();

	// Given An Array Of Face Index.
	// Sort The In CW or CCW Order.
	bool SortFaceIDs_Opted(std::vector<int> &io_vIDs,const std::map<Edge,Vertex>& mEdge2Vertex);




	// Interpolate Vertex Attributes.
	// @o_rV = @i_rV1 + (@i_rV2 - @i_rV1)*fFactor;
	void InterpolateVertexAttrib(const Vertex& i_rV1,const Vertex& i_rV2,float fFactor,Vertex& o_rV);

	// Split The Edge @rEdge By Vertex @vSplit.
	// Triangles Sharing This Edge Are Splited As Well.
	// Adjacent Information Is Updated Immediately.
	// Note !!!!!! @rVertex Must Have Already Added Into VertexMap.
	// Note !!!!!! If @o_iNewlyAddedFace Is Not Null, It Must Have Enough Root For 4 ints.
	void SplitEdge(const Edge& rEdge,const Vertex& rVertex,int *o_iNewlyAddedFace = 0);

	// Find Face "@iHost" 's Neighbor Face Who Sharing Edge "@rEdge" With Him.
	int GetNeighborFaceByEdge(int iHost,const Edge& rEdge);


	// Subdivide Triangles In @vTriangles.
	// By Introduce 3 New Vertices In The Middle Of Every Edge.
	void SubDivisionTrianglesEx(std::vector<int>& vTriangles);

	// Mesh Modified. Collier Needs Update.
	void UpdateCollider();

	// Given Face By Its Index @iFaceID And Start/End Points By Its Index @iV1,@iV2
	// Calculate The Face Sharing This Edge. Result @Return.
	int GetNeighborFaceBySharedEdge(int iFaceID,int iV1,int iV2);

	// Given Faces By Their IDs @viTriangles.
	// Calculate Their Neighbor Faces. Results @refEdgeAdj.
	void CaculateAdjacentByEdge(std::vector<std::vector<int> >& refEdgeAdj,const std::vector<int>& viTriangles);


	// A Face To Split. Its Index Is @iFace2Split.
	// Splitted By Point On Its Edge. More Info. @SplitVertices.
	void SplitFaceByVertices(int iFace2Split,const std::vector<MidVertex>& SplitVertices);




	// Correct The Winding For Face @iFace To Make It's Normal Align With @refNormal.
	void CorrectWinding(int iFace,const iv::vec3& refNormal);

	
	// Make Up Index And Vertex Array For Export.
	void MakeThingsRight( std::vector<Face>& io_vFaces,std::vector<Vertex>& io_vVertex);

	// Calculate The Intersect Point Between Sphere And Line Segment.
	// @vCenter : Center Of Sphere.
	// @fRadius : Radius Of Sphere.
	// @p1 : Point 1 Of Line Seg.
	// @p2 : Point 2 Of Line Seg.
	// @o_pFactor : float[2] Holding Roots.
	// @io_iRootNum : Ptr To float Holding Roots Count.
	void SphereVsLineSegment(const iv::vec3& vCenter,float fRadius,const iv::vec3& p1,const iv::vec3& p2,float* o_pFactor,int* io_iRootNum);


	// Create Faces That Composes This Tumor.
	// @vThatCircle : Index Of Points That Composing The Intersect Circle.
	// @fRadius : Radius Of The Tumor.
	// @iSamples : Sample Count.
	// @fSphereRadius: Radius of the hole
	void CreateTumour(float fRadius,int iSamples,float fSphereRadius,const iv::vec3& vCenter,const std::vector<int>& vThatCircle);

	// The Functions Splits Triangles That Actually Intersect With The Sphere.
	// By Intersect, I Mean Not Outside or Inside, But Truly Intersect.
	// Param @vIntersectFaceID : Indices Of Intersect Faces. 
	// @vCenter : Center Of Sphere.
	// @fRadius : Radius Of Sphere.
	void CreateIntersectionEdgeWithOutDuplication_Opted( const iv::vec3& vCenter,float fRadius,const std::vector<int>& vIntersectID,std::vector<int>& vFaceToDelete,const std::map<Edge,Vertex>& mEdge2Vertex);


	void AverageSamplePoints(const iv::vec3& vCenter,float fRadius,const std::vector<int> &vCircle);


	// Aux Functions For Narrowing.
	bool IsFaceCrossPlane(int iFaceID,const iv::vec3 &p,const iv::vec3 &d);
	bool IsFaceInPositiveSide(int iFaceID,const iv::vec3& vPoint,const iv::vec3& vPositive);

	bool FindFirstCircle(int iStart,const iv::vec3& vPoint,const iv::vec3& vDir,std::vector<int>& o_vFirstCircle);
	void FindSecondCircle(const std::vector<int>& vFirstCircle,bool* pbCheck,const iv::vec3& vPoint,const iv::vec3& vDir,std::vector<int>& o_vSecondCircle);
	void MoveFaceToCenter(const std::vector<std::vector<int> >& vFace);

	void _sortHoleCircles(std::set<Edge>& edegPool);

	//void _fillHole(const std::set<int> holeVertices);

private:
	bool		m_bEdgeAdjacency;
	bool		m_bFaceAdjacency;
	bool		m_bVertexAdjacency;


	iv::mat4	m_ModelMatrix;

	bool		m_bAdjacentyComplete;
	bool		mbStarted;
	double		mStartTime;
	double		mDuration;
	GLuint		mhVao;
	GLuint		mhVbo[3];
	bool		mbGpuLoaded;

	std::shared_ptr<CxLines>	m_EdgeLines;
	std::shared_ptr<CxLines>	m_ActiveEdgeLines;

	std::shared_ptr<CxLines>	m_SelectedFace;

	int		m_iActiveEdgeIndex;
	int		m_iSelectedFaceIndex;


	std::vector<std::vector<Edge>> m_HoleEdges;

	int			m_iErrorIndex;

	std::map<int,int>	m_DuplicateMap;

	bool		mbDirty;
	bool		mbGpuAllocated;

	std::shared_ptr<CxCollisionTest> m_pCollider;

	friend class iCathNarrowMesh;
	friend class aDynamicMesh;
	friend class CxImporter;
	friend class CxExporter;
};


