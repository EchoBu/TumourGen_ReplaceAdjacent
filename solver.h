#pragma once
#ifndef _ARAP_DEMO_SOLVER_H_
#define _ARAP_DEMO_SOLVER_H_

// C++ standard library
#include <unordered_map>

#include "Eigen/Dense"
#include "Eigen/Sparse"

#include "energy.h"

namespace arap {
	namespace demo {

		typedef std::unordered_map<int, int> Neighbors;

		enum VertexType {
			Fixed,
			Free,
		};

		// VertexInfo 用来构建从顶点id到fixed_ id 和 free_ id 的映射
		// type_ ：自由or固定
		// pos_： 顶点free_ 或者 fixed_中的位置.
		struct VertexInfo {
			// Default constructor.
			VertexInfo()
				: type(VertexType::Free),
				pos(-1) {}
			VertexInfo(VertexType type_in, int pos_in)
				: type(type_in),
				pos(pos_in) {}
			VertexType type;
			int pos;
		};

		class Solver {
		public:
			// 为Solver准备数据。
			Solver(const Eigen::MatrixXd& vertices, const Eigen::MatrixXi& faces,
				const Eigen::VectorXi& fixed, int max_iteration);

			virtual ~Solver() {}

			// 预计算并存储算法需要的参数
			virtual void Precompute() = 0;

			// 进行ARAP变形计算的主函数. 
			// @ fixed_vertices是一个三列的矩阵，存贮固定点坐标.
			virtual void Solve(const Eigen::MatrixXd& fixed_vertices);//

			// 预处理
			virtual void SolvePreprocess(const Eigen::MatrixXd& fixed_vertices) = 0;
			virtual void SolvePreprocessSim(const Eigen::MatrixXd& fixed_vertices) = 0;

			// 主函数的一次迭代过程
			virtual void SolveOneIteration() = 0;
			// 用于所有迭代完成后的后处理
			virtual void SolvePostprocess() = 0;

			// 获取 vertices_updated_.
			const Eigen::MatrixXd& GetVertexSolution() const;
			// 获取 faces_.
			const Eigen::MatrixXi& GetFaces() const;
			// 获取固定点索引
			const Eigen::VectorXi& GetFixedIndices() const;
			// 获取max_iteration_
			int GetMaxIteration() const;			
			// 计算刚性能量
			virtual Energy ComputeEnergy() const = 0;			

			
			//在给定的vertices_，vertices_updated_ 和 weight_下进行SVD投影，计算旋转矩阵rotations_
			void RefineRotations();

			
			//在给定的rotations_，fixed_vertices_, vertices_ and weight_
			//下计算更新vertices_updated_
			void RefineVertices();		

			

		protected:
			// vertices_存贮了所有顶点的位置，n行3列
			// vertices_中每一行代表一个顶点位置
			const Eigen::MatrixXd vertices_;

			
			//vertices_updated_存储Solve函数的解，维度与vertices_相同
			Eigen::MatrixXd vertices_updated_;

			
			// fixed_vertices_存储固定点坐标
			Eigen::MatrixXd fixed_vertices_;

			
			// faces_是一个存储面的信息的矩阵，每一行包含一个面的三个顶点的索引
			const Eigen::MatrixXi faces_;

			// fixed_存储了的在变形中固定的点的索引，
			const Eigen::VectorXi fixed_;

			
			// fixed_存储了的在变形中自由的点的索引，
			Eigen::VectorXi free_;
			
			// vertex_info_存储了所有点的信息
			std::vector<VertexInfo> vertex_info_;

			// 存储weight的稀疏矩阵. 		
			Eigen::SparseMatrix<double> weight_;
			// 存储每个顶点的旋转矩阵.
			std::vector<Eigen::Matrix3d> rotations_;
			// 存储每个顶点的邻接点信息
			std::vector<Neighbors> neighbors_;
			// ARAP迭代执行的次数.
			const int max_iteration_;
		};

		//return vertices_updated_
		inline const Eigen::MatrixXd& Solver::GetVertexSolution() const {
			return vertices_updated_;
		}

		inline const Eigen::MatrixXi& Solver::GetFaces() const {
			return faces_;
		}

		inline const Eigen::VectorXi& Solver::GetFixedIndices() const {
			return fixed_;
		}

		inline int Solver::GetMaxIteration() const {
			return max_iteration_;
		}
	}  // namespace demo
}  // namespace arap

#endif  // _ARAP_DEMO_SOLVER_H_
