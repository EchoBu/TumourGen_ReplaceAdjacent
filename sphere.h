#pragma once

#include <glew.h>
#include <vector>
#include <math.h>
#include <fstream>
#include <string>
#include <iostream>
#include "SiMath.h"

class GLSLProgram;

class WxSphere
{
	struct MyPoint
	{
		float x;
		float y;
		float z;
		MyPoint(float m,float n,float p):x(m),y(n),z(p) {}

		MyPoint operator+(const MyPoint& p1)
		{
			return MyPoint(this->x + p1.x,this->y + p1.y,this->z + p1.z);
		}

		MyPoint operator/(float d)
		{
			return MyPoint(this->x / d,this->y /d,this->z / d);
		}

		bool operator==(const MyPoint& rhs)
		{
			if(fabs(this->x - rhs.x) < 0.0000001 && fabs(this->y - rhs.y) < 0.0000001 && fabs(this->z - rhs.z) < 0.0000001)
				return true;
			else
				return false;
		}

		bool operator==(const MyPoint& rhs) const
		{
			if(fabs(this->x - rhs.x) < 0.0000001 && fabs(this->y - rhs.y) < 0.0000001 && fabs(this->z - rhs.z) < 0.0000001)
				return true;
			else
				return false;
		}

		void normalize()
		{
			float square = sqrt(x * x + y * y + z * z);
			if(fabs(square) < 0.00001)
			{
				x = 0.0;
				y = 0.0;
				z = 0.0;
			}
			else
			{
				x = x / square;
				y = y / square;
				z = z / square;
			}
		}
	};


	struct MyFace
	{
		unsigned int p1;
		unsigned int p2;
		unsigned int p3;
		MyFace(unsigned int m,unsigned int n,unsigned int p):p1(m),p2(n),p3(p) {}
	};
private:


	std::vector<MyPoint> m_vVertices;
	std::vector<MyPoint> m_vNormals;
	std::vector<MyFace> m_vFaces;

	float m_radius;
	GLuint vao;
	GLuint vbo[4];
	unsigned int m_nVertices;
	unsigned int m_nFaces;
	iv::mat4 model;

	iv::vec3 m_vPosition;

	void storeVBO();
	void UpdateData();
	void tess();
	int findPoint(const std::vector<MyPoint>& source,const MyPoint& des);

public:
	WxSphere(float radius = 1.0);
	~WxSphere();

	iv::mat4 getModelMatrix()
	{
		return model;
	}

	iv::vec3 GetPosition() const;

	void setPosition(iv::vec3 pos);

	void SetRadius(float fRadius);

	float GetRadius() const;

	void render();
	void renderInstances(int numInstances);

	unsigned int getWxSphereHandle()
	{
		return vao;
	}

	unsigned int getNumFaces() { return m_nFaces;}
};
