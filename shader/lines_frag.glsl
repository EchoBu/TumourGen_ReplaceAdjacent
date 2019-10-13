#version 400

uniform vec3 LineColor;



layout ( location = 0 ) out vec4 FragColor;

void main()
{
	FragColor = vec4(LineColor,1.0);
}
