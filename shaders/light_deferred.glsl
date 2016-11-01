#version 400 core

#include "atmosphere/common.h"

uniform mat4 u_InvViewProjMatrix;
uniform mat4 u_InvViewProjOriginMatrix;

uniform sampler2D s_Tex0;
uniform sampler2D s_Tex1;
uniform sampler2D s_Tex2;
uniform sampler2D s_Tex3;

uniform sampler2DArray s_SunDepthTex;
uniform mat4 u_SunShadowMatrix[4];
uniform vec4 u_SunShadowSplits;

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

vec3 GetWorldPos(vec2 uv, float depth)
{
	vec4 pos = vec4(uv, depth, 0.0) * 2.0 - 1.0;
	pos.w = 1.0;

	pos = u_InvViewProjMatrix * pos;
	pos /= pos.w;

	return pos.xyz;
}

vec3 GetWorldOriginPos(vec2 uv, float depth)
{
	vec4 pos = vec4(uv, depth, 0.0) * 2.0 - 1.0;
	pos.w = 1.0;

	pos = u_InvViewProjOriginMatrix * pos;
	pos /= pos.w;

	return pos.xyz;
}

float textureCompare(sampler2DArray depths, float index, vec2 uv, float compare)
{
	float depth = texture(depths, vec3(uv, index)).r;
	return step(compare, depth);
}

float textureShadowLerp(sampler2DArray depths, float index, vec2 size, vec2 uv, float compare)
{
	vec2 texelSize = vec2(1.0)/size;
	vec2 f = fract(uv*size+0.5);
	vec2 centroidUV = floor(uv*size+0.5)/size;
	float lb = textureCompare(depths, index, centroidUV+texelSize*vec2(0.0, 0.0), compare);
	float lt = textureCompare(depths, index, centroidUV+texelSize*vec2(0.0, 1.0), compare);
	float rb = textureCompare(depths, index, centroidUV+texelSize*vec2(1.0, 0.0), compare);
	float rt = textureCompare(depths, index, centroidUV+texelSize*vec2(1.0, 1.0), compare);
	float a = mix(lb, lt, f.y);
	float b = mix(rb, rt, f.y);
	float c = mix(a, b, f.x);
	return c;
}

float PCF(sampler2DArray depths, float index, vec2 size, vec2 uv, float compare)
{
	float result = 0.0;
	for(int x=-1; x<=1; x++){
		for(int y=-1; y<=1; y++){
			vec2 off = vec2(x,y)/size;
			result += textureShadowLerp(depths, index, size, uv+off, compare);
		}
	}

	return result/9.0;
}

float GetShadowFactor()
{
	float depth = texture(s_Tex0, fs_TexCoord).r;	
	vec3 pos = GetWorldPos(fs_TexCoord, depth);
	
	int index = 0;
	if (depth < u_SunShadowSplits.x) {
		index = 0;
	}
	else if (depth < u_SunShadowSplits.y) {
		index = 1;
	}
	else if (depth < u_SunShadowSplits.z) {
		index = 2;
	}
	else if (depth < u_SunShadowSplits.w) {
		index = 3;
	}
	else {
		return 1.0;
	}
	
	vec4 shadow_coord = u_SunShadowMatrix[index] * vec4(pos, 1);	
	float illuminated = PCF(s_SunDepthTex, index, vec2(4096.0, 4096.0), shadow_coord.xy, shadow_coord.z);
	return illuminated;
}

void main()
{
	vec4 diffuse = texture(s_Tex1, fs_TexCoord);
	float depth = texture(s_Tex0, fs_TexCoord).r;
	
	if (depth < 0.99998) {
		vec4 geometric = texture(s_Tex2, fs_TexCoord);
		vec3 p = GetWorldOriginPos(fs_TexCoord, depth);
		p += u_CameraPos;
		
		float ao = 1.0 - texture(s_Tex3, fs_TexCoord).r;
		
		vec3 color = surfaceLighting(p, diffuse.rgb, u_CameraPos, u_SunDir, geometric.rgb, geometric.a, ao * GetShadowFactor());		
		diffuse.rgb = color;
	}
	
	frag = vec4(diffuse.rgb, diffuse.a);
}