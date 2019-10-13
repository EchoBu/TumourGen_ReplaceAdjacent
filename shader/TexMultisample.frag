//this a fragment shader for general usage
#version 400

in vec3 Position;
in vec3 Normal;
in vec2 Coord;

layout (location = 0) out vec4 FragColor;


//Light parameters
struct LightInfo
{
	vec3 Intensity;
	vec4 Position;
};

uniform LightInfo light;

//Material parameters

struct MaterialInfo
{
	vec3 Kd;
	vec3 Ka;
	vec3 Ks;
	float Shininess;
};
uniform MaterialInfo material;

vec3 ads()
{
	if(gl_FrontFacing)
	{
		vec3 s = normalize(vec3(light.Position) - Position);
		vec3 v = normalize(vec3(-Position));

		vec3 norm = normalize(Normal);

		vec3 r = reflect(-s,norm);

	
		return light.Intensity * (material.Ka + 1.0 * (material.Kd * max(dot(s,Normal),0.0) + material.Ks * pow( max(dot(r,v),0.0),material.Shininess)));
	}
	else
	{
//		discard;
	
		vec3 s = normalize(vec3(light.Position) - Position);
		vec3 v = normalize(vec3(-Position));

		vec3 norm = normalize(Normal);
		vec3 r = reflect(-s,-norm);
	
		return light.Intensity * (vec3(0.0f,0.7f,0.0f) + (vec3(0.0f,0.7f,0.0f) * max(dot(s,-Normal),0.0) + material.Ks * pow( max(dot(r,v),0.0),material.Shininess)));
	}

}

void main(void)
{
	FragColor = vec4(ads(),1.0);
}

