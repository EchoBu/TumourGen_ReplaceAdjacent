#include "stdafx.h"
#include "sphere.h"
using namespace std;

WxSphere::WxSphere(float radius):m_radius(radius)
{

	m_vVertices.push_back(MyPoint(0.0, 1.0, 0.0));
	m_vVertices.push_back(MyPoint(0.0, 0.0, 1.0));
	m_vVertices.push_back(MyPoint(1.0, 0.0, 0.0));
	m_vVertices.push_back(MyPoint(0.0, 0.0, -1.0));
	m_vVertices.push_back(MyPoint(-1.0, 0.0, 0.0));
	m_vVertices.push_back(MyPoint(0.0, -1.0, 0.0));

	m_vNormals.push_back(MyPoint(0.0, 1.0, 0.0));
	m_vNormals.push_back(MyPoint(0.0, 0.0, 1.0));
	m_vNormals.push_back(MyPoint(1.0, 0.0, 0.0));
	m_vNormals.push_back(MyPoint(0.0, 0.0, -1.0));
	m_vNormals.push_back(MyPoint(-1.0, 0.0, 0.0));
	m_vNormals.push_back(MyPoint(0.0, -1.0, 0.0));
	
	m_vFaces.push_back(MyFace(0,1,2));
	m_vFaces.push_back(MyFace(0,2,3));
	m_vFaces.push_back(MyFace(0,3,4));
	m_vFaces.push_back(MyFace(0,4,1));

	m_vFaces.push_back(MyFace(5,2,1));
	m_vFaces.push_back(MyFace(5,3,2));
	m_vFaces.push_back(MyFace(5,4,3));
	m_vFaces.push_back(MyFace(5,1,4));

	m_nFaces = m_vFaces.size();
	m_nVertices = m_vVertices.size();
	for(int i = 0 ; i < 4 ; i ++)
		tess();
	storeVBO();
};

WxSphere::~WxSphere()
{
	if(vao > 0)
		glDeleteBuffers(1,&vao);
	for(int i = 0 ; i < 2 ; i++)
	{
		if(vbo[i] > 0)
			glDeleteBuffers(1,&vbo[i]);
	}

}

void WxSphere::setPosition(iv::vec3 pos)
{
	model = iv::translate(pos);
	m_vPosition = pos;
}

#if 1

void WxSphere::tess()
{
	int prevNbFaces = m_vFaces.size();
	int prevNbVertices = m_vVertices.size();
	int cIndex = prevNbVertices;
	for(int i = 0 ; i < prevNbFaces ; i++)
	{
		unsigned int f1 = m_vFaces[i].p1;
		unsigned int f2 = m_vFaces[i].p2;
		unsigned int f3 = m_vFaces[i].p3;
		MyPoint p1 = m_vVertices[f1];
		MyPoint p2 = m_vVertices[f2];
		MyPoint p3 = m_vVertices[f3];
		MyPoint p12 = ( p1 + p2) / 2.0;  //新产生的第一个点，是p1和p2的中点
		p12.normalize();
		int i12 = findPoint(m_vVertices,p12);
		if(i12 == -1)  // point p12 appears for the first time
		{
			i12 = cIndex++;
			m_vVertices.push_back(p12);
			m_vNormals.push_back(p12);
		}
		//int i12 = cIndex ++;
		MyPoint p13 = (p1 + p3) / 2.0;		//新产生的第二个点，是p1和p3的中点
		p13.normalize();
		int i13 = findPoint(m_vVertices,p13);
		if(i13 == -1)
		{
			i13 = cIndex++;
			m_vVertices.push_back(p13);
			m_vNormals.push_back(p13);
		}
		//int i13 = cIndex ++;
		MyPoint p23 = (p2 + p3) / 2.0;		//新产生的第三个点，是p2和p3的中点
		p23.normalize();
		int i23 = findPoint(m_vVertices,p23);
		if(i23 == -1)
		{
			i23 = cIndex++;
			m_vVertices.push_back(p23);
			m_vNormals.push_back(p23);
		}
		//int i23 = cIndex++;

		

		

		

		m_vFaces[i].p2 = i12;
		m_vFaces[i].p3 = i13;
		MyFace nf1 = MyFace(i12,f2,i23);
		MyFace nf2 = MyFace(i12,i23,i13);
		MyFace nf3 = MyFace(i13,i23,f3);
		m_vFaces.push_back(nf1);
		m_vFaces.push_back(nf2);
		m_vFaces.push_back(nf3);
		//cIndex++;
	}
	m_nVertices = m_vVertices.size();
	m_nFaces = m_vFaces.size();
}
#endif

int WxSphere::findPoint(const vector<MyPoint>& source,const MyPoint& des)
{
	int vecSize = source.size();
	int i;
	for(i = 0 ; i < vecSize ; i++)
	{
		if(source[i] == des)
			break; 
	}
	if( i == vecSize)
		return -1;
	else
		return i;
}

#if 1
void WxSphere::storeVBO()
{
	int nVertices = m_vVertices.size();
	int nFaces = m_vFaces.size();
	//outfile<<"This is a WxSphere model created by Haoyu Wang for test use"<<std::endl;
	float* vertices = new float[3*nVertices];
	float* norms = new float[3*nVertices];
	unsigned int* indices = new unsigned int[3*nFaces];
	//outfile<<"Vertices:"<<nVertices<<std::endl;
	for(int i = 0,j = 0 ; i < nVertices ; i++)
	{
		vertices[j++] = m_radius * m_vVertices[i].x;
		//outfile<<points[i].x<<" ";
		vertices[j++] = m_radius * m_vVertices[i].y;
		//outfile<<points[i].y<<" ";
		vertices[j++] = m_radius * m_vVertices[i].z;
		//outfile<<points[i].z<<std::endl;
	}


	for(int i = 0,j = 0 ; i < nVertices ; i++)
	{
		norms[j++] = m_vVertices[i].x;
		norms[j++] = m_vVertices[i].y;
		norms[j++] = m_vVertices[i].z;
	}
	//outfile<<"Faces:"<<nFaces<<std::endl;
	for(int i = 0,j = 0 ; i < nFaces ; i++)
	{
		indices[j++] = m_vFaces[i].p1;
		indices[j++] = m_vFaces[i].p2;
		indices[j++] = m_vFaces[i].p3;
		//outfile<<faces[i].p1<<" "<<faces[i].p2<<" "<<faces[i].p3<<std::endl;
	}


	glGenBuffers(3,vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, 3*nVertices*sizeof(float), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, 3*nVertices*sizeof(float), norms, GL_STATIC_DRAW);





	glGenVertexArrays(1,&vao);
	glBindVertexArray(vao);


	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER,vbo[0]);
	glVertexAttribPointer((GLuint)0,3,GL_FLOAT,false,0,((GLubyte*)0 + 0));

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER,vbo[1]);
	glVertexAttribPointer( (GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, ((GLubyte *)0 + (0)) );

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3*nFaces*sizeof(unsigned int), indices, GL_STATIC_DRAW);

	glBindVertexArray(0);

	delete[] vertices;
	delete[] norms;
	delete[] indices;
}
#endif

void WxSphere::render()
{
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES,3*m_nFaces,GL_UNSIGNED_INT,0);
	glBindVertexArray(0);
}

void WxSphere::renderInstances(int numInstances)
{
	if(0)
	{
		glEnable(GL_BLEND);
		glDepthMask(GL_FALSE);
		glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_COLOR);
	}
	/*if(pProg)
		pProg->setMaterial(mMaterial);*/
	glBindVertexArray(vao);
	glDrawElementsInstanced(GL_TRIANGLES, 3 * m_nFaces, GL_UNSIGNED_INT, 0, numInstances);
	if(0)
	{
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}
}

void WxSphere::SetRadius( float fRadius )
{
	m_radius = fRadius;
	UpdateData();
}

void WxSphere::UpdateData()
{
	int nVertices = m_vVertices.size();
	int nFaces = m_vFaces.size();

	float* vertices = new float[3*nVertices];
	float* norms = new float[3*nVertices];
	unsigned int* indices = new unsigned int[3*nFaces];

	for(int i = 0,j = 0 ; i < nVertices ; i++)
	{
		vertices[j++] = m_radius * m_vVertices[i].x;
		vertices[j++] = m_radius * m_vVertices[i].y;
		vertices[j++] = m_radius * m_vVertices[i].z;
	}


	for(int i = 0,j = 0 ; i < nVertices ; i++)
	{
		norms[j++] = m_vVertices[i].x;
		norms[j++] = m_vVertices[i].y;
		norms[j++] = m_vVertices[i].z;
	}

	for(int i = 0,j = 0 ; i < nFaces ; i++)
	{
		indices[j++] = m_vFaces[i].p1;
		indices[j++] = m_vFaces[i].p2;
		indices[j++] = m_vFaces[i].p3;
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferSubData(GL_ARRAY_BUFFER,0, 3*nVertices*sizeof(float), vertices);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferSubData(GL_ARRAY_BUFFER,0, 3*nVertices*sizeof(float), norms);


	glBindVertexArray(vao);


	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER,vbo[0]);
	glVertexAttribPointer((GLuint)0,3,GL_FLOAT,false,0,((GLubyte*)0 + 0));

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER,vbo[1]);
	glVertexAttribPointer( (GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, ((GLubyte *)0 + (0)) );

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[2]);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,0,3*nFaces*sizeof(unsigned int), indices);

	glBindVertexArray(0);

	delete[] vertices;
	delete[] norms;
	delete[] indices;
}

float WxSphere::GetRadius() const
{
	return m_radius;
}

iv::vec3 WxSphere::GetPosition() const
{
	return m_vPosition;
}
