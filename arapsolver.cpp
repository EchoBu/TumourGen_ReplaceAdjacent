#include "stdafx.h"
#include "arapsolver.h"

// C++ standard library
#include <iostream>
#include <vector>

#include "igl/slice.h"
#include <igl/polar_svd3x3.h>

namespace arap {
	namespace demo {

		const double kMatrixDiffThreshold = 1e-6;

		ArapSolver::ArapSolver(const Eigen::MatrixXd& vertices,
			const Eigen::MatrixXi& faces, const Eigen::VectorXi& fixed,
			int max_iteration)
			: Solver(vertices, faces, fixed, max_iteration) {

		}

		void ArapSolver::Precompute() {

			int vertex_num = vertices_.rows();
			int face_num = faces_.rows();

			// 计算weight_
			weight_.resize(vertex_num, vertex_num);

			
			// 从三角形中一个点到其对边的映射
			int index_map[3][2] = { { 1, 2 },{ 2, 0 },{ 0, 1 } };

			// 循环所有面.			
			for (int f = 0; f < face_num; ++f) {
				
				// 得到当前面三个角的余切值
				Eigen::Vector3d cotangent = ComputeCotangent(f);
				// 循环当前三角形的三个顶点.
				// i = 0 => A.
				// i = 1 => B.
				// i = 2 => C.
				for (int i = 0; i < 3; ++i) {					
					// 顶点i的对边上的两个点的索引值
					int first = faces_(f, index_map[i][0]);
					int second = faces_(f, index_map[i][1]);
					double half_cot = cotangent(i) / 2.0;
					weight_.coeffRef(first, second) += half_cot;
					weight_.coeffRef(second, first) += half_cot;					
					// weight_(i, i)是所有-weight_(i, j)的和
					weight_.coeffRef(first, first) -= half_cot;
					weight_.coeffRef(second, second) -= half_cot;
				}
			}

			// Compute neighbors.计算邻接点
			neighbors_.resize(vertex_num, Neighbors());
			for (int f = 0; f < face_num; ++f) {
				for (int i = 0; i < 3; ++i) {
					// 顶点i的对边上的两个点的索引值
					int first = faces_(f, index_map[i][0]);
					int second = faces_(f, index_map[i][1]);
					neighbors_[first][second] = second;
					neighbors_[second][first] = first;
				}
			}

			// 计算 lb_operator_.
			int free_num = free_.size();
			lb_operator_.resize(free_num, free_num);
			for (int i = 0; i < free_num; ++i) {
				// pos 是点在vertices_中的位置
				int pos = free_(i);
				// 循环所有邻接点
				for (auto& neighbor : neighbors_[pos]) {
					// 得到邻接点的索引.
					int neighbor_pos = neighbor.first;
					//得到点pos 和neighbor_pos的权重
					double weight = weight_.coeff(pos, neighbor_pos);
					lb_operator_.coeffRef(i, i) += weight;
					if (vertex_info_[neighbor_pos].type == VertexType::Free) {
						lb_operator_.coeffRef(i, vertex_info_[neighbor_pos].pos) -= weight;
					}
				}
			}
			lb_operator_.makeCompressed();
			

			// LU分解
			solver_.compute(lb_operator_);
			if (solver_.info() != Eigen::Success) {
				// lb_operator_分解失败
				std::cout << "Fail to do LU factorization." << std::endl;
				exit(EXIT_FAILURE);
			}
		}

		void ArapSolver::SolvePreprocess(const Eigen::MatrixXd& fixed_vertices) {
			
			std::cout << "SolvePreprocess" << std::endl;
			// 初始化fixed_vertices_.
			fixed_vertices_ = fixed_vertices;
			
			// 采用朴素拉普拉斯编辑得到vertices_updated_的初始猜测
			// 即在固定点位置确定的情况下最小化||Lp' - Lp||^2 			
			int vertex_num = vertices_.rows();
			int fixed_num = fixed_.size();
			int free_num = free_.size();
			int dims = vertices_.cols();
			// 初始化vertices_updated_.
			vertices_updated_.resize(vertex_num, dims);			

			// 令L=-weight_，用y表示固定点			
			// ||Lp' - Lp|| => ||-Ax - By - Lp|| => ||Ax - (-By + weight_ * p)||
			// => A'Ax = A'(-By + weight_ * p).

			// 首先构建A和B
			Eigen::SparseMatrix<double> A, B;
			Eigen::VectorXi r;						
			igl::colon<int>(0, vertices_.rows() - 1, r);				
			//在weight_矩阵中提取一些列构成新的矩阵A，B
			igl::slice(weight_, r, free_, A);//					
			igl::slice(weight_, r, fixed_, B);		

			// 构建 A' * A.
			Eigen::SparseLU<Eigen::SparseMatrix<double>> naive_lap_solver;
			Eigen::SparseMatrix<double> left = A.transpose() * A;
			left.makeCompressed();
			naive_lap_solver.compute(left);
			if (naive_lap_solver.info() != Eigen::Success) {
				std::cout << "Fail to decompose naive Laplacian function." << std::endl;
				exit(EXIT_FAILURE);
			}			
			
			//构建 A' * (-By + weight_ * p).
			for (int c = 0; c < dims; ++c) {				
				
				Eigen::VectorXd b = weight_ * vertices_.col(c) - B * fixed_vertices_.col(c);	
				Eigen::VectorXd right = A.transpose() * b;
				Eigen::VectorXd x = naive_lap_solver.solve(right);				

				if (naive_lap_solver.info() != Eigen::Success) {
					std::cout << "Fail to solve naive Laplacian function." << std::endl;
					exit(EXIT_FAILURE);
				}
				// 检查结果
				if ((left * x - right).squaredNorm() > kMatrixDiffThreshold) {
					std::cout << "Wrong in the naive Laplacian solver." << std::endl;
					return;
				}

				// 更新vertices_updated_中自由点
				for (int i = 0; i < free_num; ++i) {
					vertices_updated_(free_(i), c) = x(i);	
					
				}								
			}			

			// 写回vertices_updated_中固定点
			for (int i = 0; i < fixed_num; ++i) {
				vertices_updated_.row(fixed_(i)) = fixed_vertices_.row(i);
			}			

			// 用vertices_updated_初始化rotations_
			std::vector<Eigen::Matrix3d> edge_product(vertex_num,
				Eigen::Matrix3d::Zero());
			for (int i = 0; i < vertex_num; ++i) {
				for (auto& neighbor : neighbors_[i]) {
					int j = neighbor.first;
					double weight = weight_.coeff(i, j);
					Eigen::Vector3d edge = vertices_.row(i) - vertices_.row(j);
					Eigen::Vector3d edge_update =
						vertices_updated_.row(i) - vertices_updated_.row(j);
					edge_product[i] += weight * edge * edge_update.transpose();
				}
			}
			rotations_.clear();
			for (int v = 0; v < vertex_num; ++v) {
				Eigen::Matrix3d rotation;
				igl::polar_svd3x3(edge_product[v], rotation);
				rotations_.push_back(rotation.transpose());
			}
		}

		void ArapSolver::SolvePreprocessSim(const Eigen::MatrixXd& fixed_vertices)
		{
			std::cout << "SolvePreprocessSim" << std::endl;
			
			fixed_vertices_ = fixed_vertices;

			
			int vertex_num = vertices_.rows();
			int fixed_num = fixed_.size();
			int free_num = free_.size();
			int dims = vertices_.cols();
			// 初始化vertices_updated_.
			vertices_updated_.resize(vertex_num, dims);			

			
			for (int c = 0; c < dims; ++c) {
				// free vertices
				for (int i = 0; i < free_num; ++i) {
					vertices_updated_(free_(i), c) = vertices_(free_(i), c);
					
				}
			}

			// fixed vertices
			for (int i = 0; i < fixed_num; ++i) {
				vertices_updated_.row(fixed_(i)) = fixed_vertices_.row(i);
			}

			// 用vertices_updated_初始化rotations_
			std::vector<Eigen::Matrix3d> edge_product(vertex_num,
				Eigen::Matrix3d::Zero());
			for (int i = 0; i < vertex_num; ++i) {
				for (auto& neighbor : neighbors_[i]) {
					int j = neighbor.first;
					double weight = weight_.coeff(i, j);
					Eigen::Vector3d edge = vertices_.row(i) - vertices_.row(j);
					Eigen::Vector3d edge_update =
						vertices_updated_.row(i) - vertices_updated_.row(j);
					edge_product[i] += weight * edge * edge_update.transpose();
				}
			}
			rotations_.clear();
			for (int v = 0; v < vertex_num; ++v) {
				Eigen::Matrix3d rotation;
				igl::polar_svd3x3(edge_product[v], rotation);
				rotations_.push_back(rotation.transpose());
			}
		}
		
		void ArapSolver::SolveOneIteration() {			

			int fixed_num = fixed_.size();
			int vertex_num = vertices_.rows();
			int face_num = faces_.rows();
			
			// edge_product 是论文中等式（5）中的S
			std::vector<Eigen::Matrix3d> edge_product;

			// Step 1: 通过polar_svd求解rotations_			
			edge_product.clear();
			edge_product.resize(vertex_num, Eigen::Matrix3d::Zero());
			for (int i = 0; i < vertex_num; ++i) {
				for (auto& neighbor : neighbors_[i]) {
					int j = neighbor.first;
					double weight = weight_.coeff(i, j);
					Eigen::Vector3d edge = vertices_.row(i) - vertices_.row(j);
					Eigen::Vector3d edge_update =
						vertices_updated_.row(i) - vertices_updated_.row(j);
					edge_product[i] += weight * edge * edge_update.transpose();
				}
			}
			for (int v = 0; v < vertex_num; ++v) {
				Eigen::Matrix3d rotation;
				igl::polar_svd3x3(edge_product[v], rotation);
				rotations_[v] = rotation.transpose();
			}



			// Step 2: 计算（9）式
			int free_num = free_.size();			
			// rhs是等式（9）中的右侧
			Eigen::MatrixXd rhs = Eigen::MatrixXd::Zero(free_num, 3);
			for (int i = 0; i < free_num; ++i) {
				int i_pos = free_(i);
				for (auto& neighbor : neighbors_[i_pos]) {
					int j_pos = neighbor.first;
					double weight = weight_.coeff(i_pos, j_pos);
					Eigen::Vector3d vec = weight / 2.0
						* (rotations_[i_pos] + rotations_[j_pos])
						* (vertices_.row(i_pos) - vertices_.row(j_pos)).transpose();
					rhs.row(i) += vec;
					if (vertex_info_[j_pos].type == VertexType::Fixed) {
						rhs.row(i) += weight * vertices_updated_.row(j_pos);
					}
				}
			}
			// 求解free_.
			Eigen::MatrixXd solution = solver_.solve(rhs);
			if (solver_.info() != Eigen::Success) {
				std::cout << "Fail to solve the sparse linear system." << std::endl;
				exit(EXIT_FAILURE);
			}
			for (int i = 0; i < free_num; ++i) {
				int pos = free_(i);
				vertices_updated_.row(pos) = solution.row(i);
			}
		}

		void ArapSolver::SolvePostprocess() {
			// Simply do nothing.
			return;
		}

		Eigen::Vector3d ArapSolver::ComputeCotangent(int face_id) const {
			Eigen::Vector3d cotangent(0.0, 0.0, 0.0);
			// 三角形定义如下:
			//            A
			//           /  \
			//        c /     \ b
			//         /        \
			//        /    a      \
			//       B--------------C
			// 其中顶点 A, B, C 分别对应着faces_(face_id, 0), faces_(face_id, 1) 和
			// faces_(face_id, 2).  返回值是(cotA, cotB, cotC)

			// 首先计算三角形面积			
			Eigen::Vector3d A = vertices_.row(faces_(face_id, 0));			
			Eigen::Vector3d B = vertices_.row(faces_(face_id, 1));
			Eigen::Vector3d C = vertices_.row(faces_(face_id, 2));
			double a_squared = (B - C).squaredNorm();
			double b_squared = (C - A).squaredNorm();
			double c_squared = (A - B).squaredNorm();
			// area = 1/2bcsinA.
			double area = (B - A).cross(C - A).norm() / 2;

			// cotA = cosA / sinA.
			// b^2 + c^2 -2bccosA = a^2, or cosA = (b^2 + c^2 - a^2) / 2bc.
			// 1/2bcsinA = area, or sinA = 2area/bc.
			// cotA = (b^2 + c^2 -a^2) / 4area.
			double four_area = 4 * area;
			cotangent(0) = (b_squared + c_squared - a_squared) / four_area;
			cotangent(1) = (c_squared + a_squared - b_squared) / four_area;
			cotangent(2) = (a_squared + b_squared - c_squared) / four_area;
			return cotangent;
		}

		Energy ArapSolver::ComputeEnergy() const {
			// Compute the energy.
			double total = 0.0;
			int vertex_num = vertices_.rows();
			for (int i = 0; i < vertex_num; ++i) {
				for (auto& neighbor : neighbors_[i]) {
					int j = neighbor.first;
					double weight = weight_.coeff(i, j);
					double edge_energy = 0.0;
					Eigen::Vector3d vec = (vertices_updated_.row(i) -
						vertices_updated_.row(j)).transpose() -
						rotations_[i] * (vertices_.row(i) -
							vertices_.row(j)).transpose();
					edge_energy = weight * vec.squaredNorm();
					total += edge_energy;
				}
			}
			Energy energy;
			std::cout << "Total Energy" << total << std::endl;
			energy.AddEnergyType("Total", total);
			return energy;
		}

	}  // namespace demo
}  // namespace arap
