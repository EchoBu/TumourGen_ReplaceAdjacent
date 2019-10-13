#pragma once

#include <string>

std::string VSWireframe =	std::string("#version 400											\n") + 
	std::string("layout(location = 0) in vec3 VertexPosition;			\n") + 
	std::string("out vec3 wPosition;									\n") +
	std::string("uniform mat4 MVP;										\n") +
	std::string("void main()											\n") +
	std::string("{														\n") +
	std::string("	wPosition = VertexPosition;							\n") +
	std::string("	gl_Position = MVP * vec4(VertexPosition,1.0f);		\n") +
	std::string("}														\n");

std::string GSWireframe =	std::string("#version 400											\n") + 
	std::string("layout ( triangles ) in;								\n") + 
	std::string("layout ( line_strip, max_vertices = 4 ) out;			\n") + 
	std::string("uniform mat4 MVP;										\n") + 
	std::string("in vec3 wPosition[];									\n") + 
	std::string("void main()											\n") + 
	std::string("{														\n") + 
	std::string("	vec3 p0 = wPosition[0];								\n") + 
	std::string("	vec3 p1 = wPosition[1];								\n") + 
	std::string("	vec3 p2 = wPosition[2];								\n") + 
	std::string("	gl_Position = MVP * vec4(p0,1.0f);					\n") + 
	std::string("	EmitVertex();										\n") + 
	std::string("	gl_Position = MVP * vec4(p1,1.0f);					\n") + 
	std::string("	EmitVertex();										\n") + 
	std::string("	gl_Position = MVP * vec4(p2,1.0f);					\n") + 
	std::string("	EmitVertex();										\n") + 
	std::string("	gl_Position = MVP * vec4(p0,1.0f);					\n") + 
	std::string("	EmitVertex();										\n") + 
	std::string("}														\n"); 

std::string FSWireframe =	std::string("#version 400											\n") + 
	std::string("layout(location = 0) out vec4 FragColor;				\n") + 
	std::string("uniform vec3 LineColor;								\n") + 
	std::string("void main()											\n") + 
	std::string("{														\n") + 
	std::string("	FragColor = vec4(LineColor,1.0f);					\n") + 
	std::string("}														\n");