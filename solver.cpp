#include "stdafx.h"

#include "solver.h"

// C++ standard library
#include <iostream>
#include <vector>

#include <igl/slice.h>
#include <igl/polar_svd3x3.h>

namespace arap {
	namespace demo {

		Solver::Solver(const Eigen::MatrixXd& vertices,
			const Eigen::MatrixXi& faces, const Eigen::VectorXi& fixed,
			int max_iteration)
			: vertices_(vertices), faces_(faces), fixed_(fixed),
			max_iteration_(max_iteration) {
			// Compute free_.
			int vertex_num = vertices_.rows();
			int fixed_num = fixed_.size();
			int free_num = vertex_num - fixed_num;
			free_.resize(free_num);

						int j = 0, k = 0;
			for (int i = 0; i < vertex_num; ++i) {
				if (j < fixed_num && i == fixed_(j)) {
					++j;
				}
				else {
					free_(k) = i;
					++k;
				}
			}
			// ���fixed_��free_�Ĵ�С
			if (j != fixed_num || k != free_num) {
				std::cout << "Fail to compute free_ in Solver: dimension mismatch."
					<< std::endl;
				return;
			}
			// ����ÿ���������Ϣ
			vertex_info_.resize(vertex_num);
			// �̶�����Ϣ
			for (int i = 0; i < fixed_num; ++i) {
				int vertex_id = fixed_(i);
				VertexInfo info(VertexType::Fixed, i);
				vertex_info_[vertex_id] = info;
			}
			// ���ɵ���Ϣ
			for (int i = 0; i < free_num; ++i) {
				int vertex_id = free_(i);
				VertexInfo info(VertexType::Free, i);
				vertex_info_[vertex_id] = info;
			}
			
			// ��鶥����Ϣ
			for (int i = 0; i < vertex_num; ++i) {
				VertexInfo info = vertex_info_[i];
				switch (info.type) {
				case Fixed:
					if (fixed_(info.pos) != i) {
						std::cout << "Fail to test vertex info: wrong fixed position."
							<< std::endl;
						return;
					}
					break;
				case Free:
					if (free_(info.pos) != i) {
						std::cout << "Fail to test vertex info: wrong free position."
							<< std::endl;
						return;
					}
					break;
				default:
					std::cout << "Unknown vertex type." << std::endl;
					return;
				}
			}
		}


		void Solver::Solve(const Eigen::MatrixXd& fixed_vertices) {			
			// �������p��R��ʵ��ARAP����
			
			//SolvePreprocessSim(fixed_vertices);
			SolvePreprocess(fixed_vertices);
			int iter = 0;
			Energy e = ComputeEnergy();
			while (iter < max_iteration_) {

				e = ComputeEnergy();
				//RefineRotations();
				//RefineVertices();
				SolveOneIteration();
				++iter;				

			}
			SolvePostprocess();

			
		}

		
		// �ڸ�����vertices_��vertices_updated_ �� weight_�½���SVDͶӰ��������ת����rotations_
		void Solver::RefineRotations() {
			int vertex_num = vertices_.rows();
			std::vector<Eigen::Matrix3d> edge_product(vertex_num, Eigen::Matrix3d::Zero());
			for (int i = 0; i < vertex_num; ++i) {
				auto& neighbor = Neighbors();
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
		}

		//�ڸ�����rotations_��fixed_vertices_, vertices_ and weight_
		//�¼������vertices_updated_
		void Solver::RefineVertices() {
			int fixed_num = fixed_.size();			
			//�̶���
			for (int i = 0; i < fixed_num; ++i) {
				vertices_updated_.row(fixed_(i)) = fixed_vertices_.row(i);
			}

			Eigen::SparseMatrix<double> lb_operator;
			igl::slice(weight_, free_, free_, lb_operator);
			lb_operator *= -1.0;
			lb_operator.makeCompressed();
			
			//LU �ֽ�
			Eigen::SparseLU<Eigen::SparseMatrix<double>> solver;
			solver.compute(lb_operator);
			if (solver.info() != Eigen::Success) {
				// lb_operator_�ֽ�ʧ��
				std::cout << "Fail to do LU factorization." << std::endl;
				exit(EXIT_FAILURE);
			}

			int free_num = free_.size();
			
			// �����е�ʽ��9�����Ҳ࣬x��y��z�����Ƿֿ������
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
			
			// ������ɵ�
			Eigen::MatrixXd solution = solver.solve(rhs);
			if (solver.info() != Eigen::Success) {
				std::cout << "Fail to solve the sparse linear system." << std::endl;
				exit(EXIT_FAILURE);
			}
			for (int i = 0; i < free_num; ++i) {
				int pos = free_(i);
				vertices_updated_.row(pos) = solution.row(i);
			}
		}
			

	}  // namespace demo
}  // namespace arap
