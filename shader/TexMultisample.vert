//vertex shader for general usage for phone light model which calculated the light data in fragment shader.
#version 400

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec2 VertexCoord;

out vec3 Position;
out vec3 Normal;
out vec2 Coord;

uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;

vec4 generalProc(void)
{
	mat4 MV = ViewMatrix * ModelMatrix;

	mat3 NormalMat = mat3(vec3(MV[0]),vec3(MV[1]),vec3(MV[2]));

	vec3 tmpNorm = VertexNormal;
	Normal = normalize(NormalMat * normalize(tmpNorm));

	//Position是在相机坐标系下的坐标
	Position = vec3(MV * vec4(VertexPosition,1.0));
	
	return ProjectionMatrix * MV * vec4(VertexPosition,1.0);
}
void main(void)
{
	gl_Position = generalProc();
}