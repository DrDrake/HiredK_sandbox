#version 400 core

#define MAX_NUM_SHADOW_SPLITS 4

#include "common.h"

uniform sampler2D s_Tex0; // depth
uniform sampler2D s_Tex1; // albedo
uniform sampler2D s_Tex2; // normal
uniform sampler2D s_Tex3; // occlusion

uniform mat4 u_LightShadowMatrix[MAX_NUM_SHADOW_SPLITS];
uniform float u_LightShadowResolution;
uniform vec4 u_LightShadowSplits;
uniform vec3 u_LightDirection;
uniform sampler2DArray s_LightDepthTex;

uniform float u_EarthRadius;
uniform float u_StartHeight;
uniform vec2 u_CoverageOffset;
uniform float u_CoverageScale;
uniform sampler2D s_Coverage;

uniform mat4 u_InvViewProjMatrix;
uniform vec3 u_WorldCamPosition;
uniform float u_DeferredDist;

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

float textureCompare(sampler2DArray depths, float index, vec2 uv, float compare)
{
	float depth = texture(depths, vec3(uv, index)).r;
	return step(compare, depth);
}

float textureShadowLerp(sampler2DArray depths, float index, vec2 size, vec2 uv, float compare)
{
	vec2 texelSize = vec2(1.0) / size;
	vec2 f = fract(uv * size + 0.5);
	vec2 centroidUV = floor(uv * size + 0.5) / size;
	
	float lb = textureCompare(depths, index, centroidUV + texelSize * vec2(0.0, 0.0), compare);
	float lt = textureCompare(depths, index, centroidUV + texelSize * vec2(0.0, 1.0), compare);
	float rb = textureCompare(depths, index, centroidUV + texelSize * vec2(1.0, 0.0), compare);
	float rt = textureCompare(depths, index, centroidUV + texelSize * vec2(1.0, 1.0), compare);	
	float a = mix(lb, lt, f.y);
	float b = mix(rb, rt, f.y);
	return mix(a, b, f.x);
}

float PCF(sampler2DArray depths, float index, vec2 size, vec2 uv, float compare)
{
	float result = 0.0;	
	for(int x = -1; x <= 1; x++)
	for(int y = -1; y <= 1; y++) {
		result += textureShadowLerp(depths, index, size, uv + (vec2(x, y) / size), compare);
	}

	return result / 9.0;
}

float GetCloudsShadowFactor(sampler2D tex, float radius, vec3 origin, vec3 direction, vec2 offset, float scale)
{
    float a0 = radius * radius - dot(origin, origin);
	float a1 = dot(origin, direction);
	float result = sqrt(a1 * a1 + a0) - a1;
    
	vec3 intersect = origin + direction * result;	
	vec2 unit = intersect.xz * scale;
	vec2 coverageUV = unit * 0.5 + 0.5;
	coverageUV += offset;
	
	vec4 coverage = texture(tex, coverageUV);
	float cloudShadow = coverage.r;	
	cloudShadow *= 0.5;
	
	return 1.0 - cloudShadow;
}

float GetShadowFactor(vec3 pos, float depth)
{
	float factor = GetCloudsShadowFactor(s_Coverage, u_EarthRadius + u_StartHeight, pos + vec3(0, u_EarthRadius, 0), u_LightDirection, u_CoverageOffset, u_CoverageScale);   
    
	int index = 0;
	if (depth < u_LightShadowSplits.x) {
		index = 0;
	}
	else if (depth < u_LightShadowSplits.y) {
		index = 1;
	}
	else if (depth < u_LightShadowSplits.z) {
		index = 2;
	}
	else if (depth < u_LightShadowSplits.w) {
		index = 3;
	}
	else {
		return factor;
	}
	
	vec4 shadow_coord = u_LightShadowMatrix[index] * vec4(pos, 1);	
	factor *= PCF(s_LightDepthTex, index, vec2(u_LightShadowResolution), shadow_coord.xy, shadow_coord.z);
	return factor;
}

void main()
{
    float depth = texture(s_Tex0, fs_TexCoord).r;
	vec4 albedo = texture(s_Tex1, fs_TexCoord);
	
	if (depth < u_DeferredDist)
	{
		vec4 normal = texture(s_Tex2, fs_TexCoord);
		float ao = 1.0 - texture(s_Tex3, fs_TexCoord).r;
		
		float roughness = clamp(albedo.a, 0.0, 1.0);
		float metalness = clamp(normal.a, 0.0, 1.0);
		
		vec3 wpos = GetWorldPos(fs_TexCoord, depth);
		float shadow = GetShadowFactor(wpos, depth);
		
		vec3 N = normalize(normal.xyz * 2.0 - 1.0);
		vec3 V = normalize(u_WorldCamPosition - wpos);    
		vec3 L = normalize(u_LightDirection);
		vec3 H = normalize(V + L);
		
		float ndotv = clamp(dot(N, V), 0.0, 1.0);
		float ndotl = clamp(dot(N, L), 0.0, 1.0);
		float ndoth = clamp(dot(N, H), 0.0, 1.0);
		float ldoth = clamp(dot(L, H), 0.0, 1.0);
		
		vec3 fd = BRDF_Atmospheric(wpos + vec3(0, 6360000.0, 0), N, albedo.rgb, u_WorldCamPosition + vec3(0, 6360000.0, 0), L, shadow);
		
		// temp fix this
		//vec3 sr = sunRadiance(length(wpos + vec3(0, 6360000.0, 0)), dot((wpos + vec3(0, 6360000.0, 0)) / length(wpos + vec3(0, 6360000.0, 0)), L));	
		//vec3 fs = BRDF_CookTorrance(ldoth, ndoth, ndotv, ndotl, roughness, metalness) * sr * shadow * 0.0008f;
		albedo.rgb = (fd) * ao;
	}	
	
    frag = vec4(albedo.rgb, 1);
}