#version 400 core

#include "common.h"

uniform mat4 u_ViewProjMatrix;
uniform mat4 u_ModelMatrix;
uniform mat3 u_NormalMatrix;

uniform vec2 u_TexCoordScale;
uniform bool u_VertexColor;
uniform sampler2D s_Tex0;

uniform bool u_EnableColor = false;
uniform float u_Alpha = 1.0;
uniform vec4 u_Color;

-- vs
layout(location = 0) in vec3 vs_Position;
layout(location = 1) in vec3 vs_Normal;
layout(location = 2) in vec2 vs_TexCoord;
layout(location = 3) in vec3 vs_Color;
out vec3 fs_Position;
out vec3 fs_Normal;
out vec2 fs_TexCoord;
out vec3 fs_Color;

void main()
{	
	gl_Position = u_ViewProjMatrix * u_ModelMatrix * vec4(vs_Position.xyz, 1);  
    fs_Position = vec3(u_ModelMatrix * vec4(vs_Position.xyz, 1));
	fs_Normal = u_NormalMatrix * vs_Normal;
	fs_TexCoord = vs_TexCoord;
    fs_Color = vs_Color;
}

-- fs
layout(location = 0) out vec4 frag;
in vec3 fs_Position;
in vec3 fs_Normal;
in vec2 fs_TexCoord;
in vec3 fs_Color;

void main()
{
	frag = u_Color;
}