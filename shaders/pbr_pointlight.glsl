#version 400 core

#include "common.h"

uniform sampler2D s_Tex0; // depth
uniform sampler2D s_Tex1; // albedo
uniform sampler2D s_Tex2; // normal
uniform sampler2D s_Tex3; // occlusion

uniform mat4 u_ViewProjMatrix;
uniform mat4 u_ModelMatrix;

uniform mat4 u_InvViewProjMatrix;
uniform vec3 u_WorldCamPosition;

uniform float u_LightIntensity;
uniform float u_InvLightRadius;
uniform vec3 u_LightWorldPos;
uniform vec4 u_LightColor;

-- vs
layout(location = 0) in vec3 vs_Position;
out vec3 fs_ProjCoord;

void main()
{	
	gl_Position = u_ViewProjMatrix * u_ModelMatrix * vec4(vs_Position.xyz, 1);
	fs_ProjCoord.x = (gl_Position.x + gl_Position.w) * 0.5;
	fs_ProjCoord.y = (gl_Position.y + gl_Position.w) * 0.5;
	fs_ProjCoord.z = gl_Position.w;
}

-- fs
layout(location = 0) out vec4 frag;
in vec3 fs_ProjCoord;

vec3 GetWorldPos(vec2 uv, float depth)
{
	vec4 pos = vec4(uv, depth, 0.0) * 2.0 - 1.0;
	pos.w = 1.0;

	pos = u_InvViewProjMatrix * pos;
	pos /= pos.w;

	return pos.xyz;
}

void main()
{
    float depth = textureProj(s_Tex0, fs_ProjCoord).r;
    vec4 albedo = textureProj(s_Tex1, fs_ProjCoord);
    vec4 normal = textureProj(s_Tex2, fs_ProjCoord);
    float ao = 1.0 - textureProj(s_Tex3, fs_ProjCoord).r;
    
    float roughness = clamp(albedo.a, 0.0, 1.0);
    float metalness = clamp(normal.a, 0.0, 1.0);
    
    vec2 uv = fs_ProjCoord.xy / fs_ProjCoord.z;   
    vec3 wpos = GetWorldPos(uv, depth);
    vec3 ldir = u_LightWorldPos - wpos;
    
    vec3 N = normalize(normal.xyz * 2.0 - 1.0);
    vec3 V = normalize(u_WorldCamPosition - wpos);
    vec3 L = normalize(ldir);
    vec3 H = normalize(V + L);
    
    float ndotv = clamp(dot(N, V), 0.0, 1.0);
	float ndotl = clamp(dot(N, L), 0.0, 1.0);
	float ndoth = clamp(dot(N, H), 0.0, 1.0);
	float ldoth = clamp(dot(L, H), 0.0, 1.0);
    
    vec3 fd = BRDF_Lambertian(albedo.rgb, metalness);
    vec3 fs = BRDF_CookTorrance(ldoth, ndoth, ndotv, ndotl, roughness, metalness);
    
    float dist = length(ldir);
    float dist2	= max(dot(ldir, ldir), 1e-4);
    float falloff = (u_LightIntensity / dist2) * max(0.0, 1.0 - dist * u_InvLightRadius);    
    float fade = max(0.0, (1.0 - 0.75) * 4.0);
    float shadow = mix(1.0, falloff, fade);
    
    vec3 final_color = (fd + fs) * u_LightColor.rgb * ndotl * shadow * 0.5f;
    final_color *= ao;
    
	frag = vec4(final_color, shadow);
}