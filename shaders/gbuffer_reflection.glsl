#version 400 core

#include "common.h"

uniform sampler2D s_Albedo;
uniform sampler2D s_Normal;

uniform mat4 u_ViewProjMatrix;
uniform mat4 u_ModelMatrix;
uniform mat3 u_NormalMatrix;

uniform vec3 u_WorldCamPosition;
uniform vec3 u_SunDirection;

-- vs
layout(location = 0) in vec3 vs_Position;
layout(location = 1) in vec2 vs_TexCoord;
layout(location = 2) in vec4 vs_Color;
layout(location = 3) in vec3 vs_Normal;
out vec3 fs_Position;
out vec2 fs_TexCoord;
out vec3 fs_Normal;

void main()
{
    fs_Position = vec3(u_ModelMatrix * vec4(vs_Position.xyz, 1));
	gl_Position = u_ViewProjMatrix * vec4(fs_Position, 1);
	fs_TexCoord = vs_TexCoord;
    fs_Normal = u_NormalMatrix * vs_Normal;
}

-- fs
layout(location = 0) out vec4 frag;
in vec3 fs_Position;
in vec2 fs_TexCoord;
in vec3 fs_Normal;

void main()
{
    vec4 color = texture(s_Albedo, fs_TexCoord);    
    vec3 N = normalize(fs_Normal);
    vec3 L = normalize(u_SunDirection);
    
    color.rgb = BRDF_Atmospheric(fs_Position + vec3(0, 6360000.0, 0), N, color.rgb, u_WorldCamPosition + vec3(0, 6360000.0, 0), L, 1.0);	
	frag = vec4(color.rgb * 0.1f, 1);
}