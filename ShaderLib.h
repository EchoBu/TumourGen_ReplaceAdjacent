#pragma once

class GLShader;

enum E_SHADER_TYPE {
	E_LINE_SHADER = 0,
	E_TEXTURE_MULTISAMPLE_PASS_ONE,
	E_WIREFRAME,
	E_POINTS,
	E_SHADER_NUMBER
};

class ShaderLib
{
private:
	GLShader* m_pShaderLib[E_SHADER_NUMBER];

public:
	ShaderLib();
	~ShaderLib();

	void CompileAndLinkAllShaderPrograms(void);
	GLShader* GetShaderProgram(E_SHADER_TYPE effect) const;
};