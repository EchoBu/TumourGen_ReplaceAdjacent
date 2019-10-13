#include "stdafx.h"

#include "ShaderLib.h"
#include "Shaders.h"

#include "GLShader.h"

ShaderLib::ShaderLib()
{
	for(int i = 0 ; i < E_SHADER_NUMBER ; i++)
		m_pShaderLib[i] = NULL;
}

ShaderLib::~ShaderLib()
{
	for (int i = 0 ; i < E_SHADER_NUMBER ; i++)
	{
		if(m_pShaderLib[i])
			delete m_pShaderLib[i],m_pShaderLib[i] = NULL;
	}
}

void ShaderLib::CompileAndLinkAllShaderPrograms()
{
	// setup for shaders that draw wide lines
	m_pShaderLib[E_LINE_SHADER] = new GLShader;
	m_pShaderLib[E_LINE_SHADER]->Load( "shader/lines_vert.glsl",	GLSLShader::VERTEX,
									   "shader/lines_frag.glsl",	GLSLShader::FRAGMENT );
	

	//setup for multisample texture pass one
	m_pShaderLib[E_TEXTURE_MULTISAMPLE_PASS_ONE] = new GLShader;
	m_pShaderLib[E_TEXTURE_MULTISAMPLE_PASS_ONE]->Load( "shader/TexMultisample.vert",	GLSLShader::VERTEX,
														"shader/TexMultisample.frag",	GLSLShader::FRAGMENT );

	m_pShaderLib[E_WIREFRAME] = new GLShader;
	m_pShaderLib[E_WIREFRAME]->LoadFromString(VSWireframe,GLSLShader::VERTEX,
		GSWireframe,GLSLShader::GEOMETRY,
		FSWireframe,GLSLShader::FRAGMENT);

	//setup for multisample texture pass one
	m_pShaderLib[E_POINTS] = new GLShader;
	m_pShaderLib[E_POINTS]->Load( "shader/points.vert",	GLSLShader::VERTEX,
		"shader/points.frag",	GLSLShader::FRAGMENT );
}

GLShader* ShaderLib::GetShaderProgram(E_SHADER_TYPE effect) const
{
	return m_pShaderLib[effect];
}