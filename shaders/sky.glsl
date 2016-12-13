#version 400 core

#include "atmosphere/common.h"

uniform mat4 u_InvViewProjMatrix;
uniform vec3 u_LocalWorldPos;
uniform vec3 u_SunDirection;

uniform sampler2D s_Clouds;

-- vs
layout(location = 0) in vec2 vs_Position;
layout(location = 1) in vec2 vs_TexCoord;
out vec2 fs_TexCoord;
out vec3 fs_ViewRay;
out vec3 fs_RelativeViewRay;

void main()
{
	gl_Position = vec4(vs_Position, 0, 1);
	fs_TexCoord = vs_TexCoord;
    
    fs_ViewRay = vec3(u_InvViewProjMatrix * vec4(vs_Position, 0, 1));
    
    float theta = acos(u_SunDirection.z);
    float phi = atan(u_SunDirection.y, u_SunDirection.x);    
    mat3 rz = mat3(cos(phi), -sin(phi), 0, sin(phi), cos(phi), 0, 0, 0, 1);
    mat3 ry = mat3(cos(theta), 0, sin(theta), 0, 1, 0, -sin(theta), 0, cos(theta));
    fs_RelativeViewRay = (ry * rz) * fs_ViewRay;
}

-- fs
layout(location = 0) out vec4 frag;
in vec2 fs_TexCoord;
in vec3 fs_ViewRay;
in vec3 fs_RelativeViewRay;

void main()
{
    vec4 clouds = texture(s_Clouds, fs_TexCoord);
    vec3 color = mix(outerSunRadiance(fs_RelativeViewRay) * (1.0 - clouds.a), clouds.rgb, clouds.a);
    
    vec3 extinction;
    vec3 inscatter = skyRadiance(u_LocalWorldPos + vec3(0, 6360000.0, 0), normalize(fs_ViewRay), u_SunDirection, extinction, 0.0) * 0.08;
    frag = vec4(color * extinction + inscatter, 1.0);
}