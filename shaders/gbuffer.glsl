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
out float fs_Flogz;

void main()
{	
	gl_Position = u_ViewProjMatrix * u_ModelMatrix * vec4(vs_Position.xyz, 1);  
    fs_Position = vec3(u_ModelMatrix * vec4(vs_Position.xyz, 1));
	fs_Normal = u_NormalMatrix * vs_Normal;
	fs_TexCoord = vs_TexCoord;
    fs_Color = vs_Color;
    
    if (u_EnableLogZ) {
        gl_Position.z = log2(max(1e-6, 1.0 + gl_Position.w)) * Fcoef(u_CameraClip.y);
        fs_Flogz = 1.0 + gl_Position.w;
    }
}

-- fs
layout(location = 0) out vec4 diffuse;
layout(location = 1) out vec4 geometric;
in vec3 fs_Position;
in vec3 fs_Normal;
in vec2 fs_TexCoord;
in vec3 fs_Color;
in float fs_Flogz;

void main()
{
    vec4 color = vec4(0, 0, 0, u_Alpha);
    
    if (u_EnableColor) {
        color = u_Color;
    }
    else {
        color = texture(s_Tex0, fs_TexCoord * u_TexCoordScale);
    }
    
    if (u_EnableLogZ) {
        gl_FragDepth = log2(fs_Flogz) * 0.5 * Fcoef(u_CameraClip.y);
    }
    else {
        gl_FragDepth = gl_FragCoord.z;
    }
    
	diffuse = vec4(color.rgb * 0.02, color.a * u_Alpha);
    geometric = vec4(fs_Normal * 0.5 + 0.5, 0.05);
}