#version 400 core

#include "atmosphere/common.h"

uniform sampler2D s_Clouds;

uniform mat4 u_InvProjMatrix;
uniform mat4 u_InvViewMatrix;

-- vs
layout(location = 0) in vec2 vs_Position;
layout(location = 1) in vec2 vs_TexCoord;
out vec2 fs_TexCoord;
out vec3 dir;
out vec3 relativeDir;

void main()
{
	vec3 WSD = u_SunDir;

    dir = (u_InvViewMatrix * vec4((u_InvProjMatrix * vec4(vs_Position, 0.0, 1.0)).xyz, 0.0)).xyz;

    float theta = acos(WSD.z);
    float phi = atan(WSD.y, WSD.x);
    mat3 rz = mat3(cos(phi), -sin(phi), 0.0, sin(phi), cos(phi), 0.0, 0.0, 0.0, 1.0);
    mat3 ry = mat3(cos(theta), 0.0, sin(theta), 0.0, 1.0, 0.0, -sin(theta), 0.0, cos(theta));
	
    // apply this rotation to view dir to get relative viewdir
    relativeDir = (ry * rz) * dir;

    gl_Position = vec4(vs_Position.xy, 0.0, 1.0);
	fs_TexCoord = vs_TexCoord;
}

-- fs
layout(location = 0) out vec4 frag;
in vec2 fs_TexCoord;
in vec3 dir;
in vec3 relativeDir;

void main()
{
	vec3 WSD = u_SunDir;
    vec3 WCP = u_CameraPos;

    vec3 d = normalize(dir);
	
	vec4 clouds = texture(s_Clouds, fs_TexCoord);
    vec3 sunColor = mix(outerSunRadiance(relativeDir) * (1.0 - clouds.a), clouds.rgb, clouds.a);

    vec3 extinction;
    vec3 inscatter = skyRadiance(WCP, d, WSD, extinction, 0.0) * 0.08;
    vec3 finalColor = sunColor * extinction + inscatter;

	frag = vec4(finalColor, 1.0);
}