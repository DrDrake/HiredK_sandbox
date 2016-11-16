#version 400 core

#include "common.h"

uniform mat4 u_ViewProjMatrix;
uniform mat4 u_ModelMatrix;

uniform sampler2D s_Tex0;

-- vs
layout(location = 0) in vec3 vs_Position;
layout(location = 1) in vec3 vs_Normal;
layout(location = 2) in vec2 vs_TexCoord;
out vec3 fs_Position;
out vec3 fs_Normal;
out vec2 fs_TexCoord;
out float fs_Flogz;

void main()
{	
	gl_Position = u_ViewProjMatrix * u_ModelMatrix * vec4(vs_Position.xyz, 1);  
    fs_Position = vec3(u_ModelMatrix * vec4(vs_Position.xyz, 1));
	fs_Normal = vs_Normal;
	fs_TexCoord = vs_TexCoord;
    
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
in float fs_Flogz;

void main()
{
    vec3 color = texture(s_Tex0, fs_TexCoord).rgb;
    
    if (u_EnableLogZ) {
        vec3 offset = fs_Position + vec3(0, 6360000.0, 0);
        color = surfaceLighting(offset, color * 0.25, u_CameraPos, u_SunDir, fs_Normal, 0.0, 1.0);
        gl_FragDepth = log2(fs_Flogz) * 0.5 * Fcoef(u_CameraClip.y);
    }
    else {
        gl_FragDepth = gl_FragCoord.z;
    }
    
	diffuse = vec4(color, 1.0);
    geometric = vec4(fs_Normal * 0.5 + 0.5, 0.0);
}