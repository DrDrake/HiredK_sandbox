#version 400 core

const int MAX_SAMPLES = 64;
uniform int u_Samples = 8;

uniform float u_Dispersal = 0.25;
uniform float u_HaloWidth = 1.0;
uniform float u_Distortion = 1.0;

uniform sampler2D s_Tex0;
uniform sampler1D s_Tex1;

-- vs
layout(location = 0) in vec2 vs_Position;
layout(location = 1) in vec2 vs_TexCoord;
out vec2 fs_TexCoord;

void main()
{
	gl_Position = vec4(vs_Position, 0, 1);
	fs_TexCoord = vs_TexCoord;
}

-- fs
layout(location = 0) out vec4 frag;
in vec2 fs_TexCoord;

vec4 textureDistorted(sampler2D tex, vec2 texcoord, vec2 direction, vec3 distortion)
{
	return vec4(
		texture(tex, texcoord + direction * distortion.r).r,
		texture(tex, texcoord + direction * distortion.g).g,
		texture(tex, texcoord + direction * distortion.b).b,
		1.0
	);
}

void main() {
	vec2 texcoord = -fs_TexCoord + vec2(1.0);
	vec2 texelSize = 1.0 / vec2(textureSize(s_Tex0, 0));
	
	vec2 ghostVec = (vec2(0.5) - texcoord) * u_Dispersal;
	vec2 haloVec = normalize(ghostVec) * u_HaloWidth;
	
	vec3 distortion = vec3(-texelSize.x * u_Distortion, 0.0, texelSize.x * u_Distortion);

	vec4 result = vec4(0.0);
	for (int i = 0; i < u_Samples; ++i) {
		vec2 offset = fract(texcoord + ghostVec * float(i));
		
		float weight = length(vec2(0.5) - offset) / length(vec2(0.5));
		weight = pow(1.0 - weight, 10.0);
	
		result += textureDistorted(
			s_Tex0,
			offset,
			normalize(ghostVec),
			distortion
		) * weight;
	}
	
	result *= texture(s_Tex1, length(vec2(0.5) - texcoord) / length(vec2(0.5)));

	float weight = length(vec2(0.5) - fract(texcoord + haloVec)) / length(vec2(0.5));
	weight = pow(1.0 - weight, 10.0);
	result += textureDistorted(
		s_Tex0,
		fract(texcoord + haloVec),
		normalize(ghostVec),
		distortion
	) * weight;
	
	frag = result;
}