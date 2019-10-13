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

			
			// 预计算余切权值和ARAP中左侧的矩阵
			void Precompute();

			
			// 设置初始猜测
			void SolvePreprocess(const Eigen::MatrixXd& fixed_vertices);
			void SolvePreprocessSim(const Eigen::MatrixXd& fixed_vertices);

			// 一次迭代过程
			void SolveOneIteration();
			// 迭代结束的后处理
			void SolvePostprocess();
			// 计算能量
			Energy ComputeEnergy() const;

		private:
			// 计算面|face_id|的三个角的余切值
			Eigen::Vector3d ComputeCotangent(int face_id) const;

			
			// 论文中等式（9）的左侧Laplace-Beltrami算子
			Eigen::SparseMatrix<double> lb_operator_;
			
			// 用来进行LU求解的求解器
			Eigen::SparseLU<Eigen::SparseMatrix<double>> solver_;
		};

	}  // namespace demo
}  // namespace arap

#endif  // _ARAP_DEMO_ARAPSOLVER_H_