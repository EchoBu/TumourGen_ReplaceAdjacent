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

		// VertexInfo ���������Ӷ���id��fixed_ id �� free_ id ��ӳ��
		// type_ ������or�̶�
		// pos_�� ����free_ ���� fixed_�е�λ��.
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
			// ΪSolver׼�����ݡ�
			Solver(const Eigen::MatrixXd& vertices, const Eigen::MatrixXi& faces,
				const Eigen::VectorXi& fixed, int max_iteration);

			virtual ~Solver() {}

			// Ԥ���㲢�洢�㷨��Ҫ�Ĳ���
			virtual void Precompute() = 0;

			// ����ARAP���μ����������. 
			// @ fixed_vertices��һ�����еľ��󣬴����̶�������.
			virtual void Solve(const Eigen::MatrixXd& fixed_vertices);//

			// Ԥ����
			virtual void SolvePreprocess(const Eigen::MatrixXd& fixed_vertices) = 0;
			virtual void SolvePreprocessSim(const Eigen::MatrixXd& fixed_vertices) = 0;

			// ��������һ�ε�������
			virtual void SolveOneIteration() = 0;
			// �������е�����ɺ�ĺ���
			virtual void SolvePostprocess() = 0;

			// ��ȡ vertices_updated_.
			const Eigen::MatrixXd& GetVertexSolution() const;
			// ��ȡ faces_.
			const Eigen::MatrixXi& GetFaces() const;
			// ��ȡ�̶�������
			const Eigen::VectorXi& GetFixedIndices() const;
			// ��ȡmax_iteration_
			int GetMaxIteration() const;			
			// �����������
			virtual Energy ComputeEnergy() const = 0;			

			
			//�ڸ�����vertices_��vertices_updated_ �� weight_�½���SVDͶӰ��������ת����rotations_
			void RefineRotations();

			
			//�ڸ�����rotations_��fixed_vertices_, vertices_ and weight_
			//�¼������vertices_updated_
			void RefineVertices();		

			

		protected:
			// vertices_���������ж����λ�ã�n��3��
			// vertices_��ÿһ�д���һ������λ��
			const Eigen::MatrixXd vertices_;

			
			//vertices_updated_�洢Solve�����Ľ⣬ά����vertices_��ͬ
			Eigen::MatrixXd vertices_updated_;

			
			// fixed_vertices_�洢�̶�������
			Eigen::MatrixXd fixed_vertices_;

			
			// faces_��һ���洢�����Ϣ�ľ���ÿһ�а���һ������������������
			const Eigen::MatrixXi faces_;

			// fixed_�洢�˵��ڱ����й̶��ĵ��������
			const Eigen::VectorXi fixed_;

			
			// fixed_�洢�˵��ڱ��������ɵĵ��������
			Eigen::VectorXi free_;
			
			// vertex_info_�洢�����е����Ϣ
			std::vector<VertexInfo> vertex_info_;

			// �洢weight��ϡ�����. 		
			Eigen::SparseMatrix<double> weight_;
			// �洢ÿ���������ת����.
			std::vector<Eigen::Matrix3d> rotations_;
			// �洢ÿ��������ڽӵ���Ϣ
			std::vector<Neighbors> neighbors_;
			// ARAP����ִ�еĴ���.
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
