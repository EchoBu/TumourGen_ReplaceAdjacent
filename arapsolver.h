#pragma once
#ifndef _ARAP_DEMO_ARAPSOLVER_H_
#define _ARAP_DEMO_ARAPSOLVER_H_

#include "solver.h"

#include "Eigen/SparseLU"

namespace arap {
	namespace demo {

		class  ArapSolver : public Solver {
		public:
			ArapSolver(const Eigen::MatrixXd& vertices, const Eigen::MatrixXi& faces,
				const Eigen::VectorXi& fixed, int max_iteration);

			
			// Ԥ��������Ȩֵ��ARAP�����ľ���
			void Precompute();

			
			// ���ó�ʼ�²�
			void SolvePreprocess(const Eigen::MatrixXd& fixed_vertices);
			void SolvePreprocessSim(const Eigen::MatrixXd& fixed_vertices);

			// һ�ε�������
			void SolveOneIteration();
			// ���������ĺ���
			void SolvePostprocess();
			// ��������
			Energy ComputeEnergy() const;

		private:
			// ������|face_id|�������ǵ�����ֵ
			Eigen::Vector3d ComputeCotangent(int face_id) const;

			
			// �����е�ʽ��9�������Laplace-Beltrami����
			Eigen::SparseMatrix<double> lb_operator_;
			
			// ��������LU���������
			Eigen::SparseLU<Eigen::SparseMatrix<double>> solver_;
		};

	}  // namespace demo
}  // namespace arap

#endif  // _ARAP_DEMO_ARAPSOLVER_H_