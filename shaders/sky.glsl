#version 400 core

#include "atmosphere/common.h"

uniform mat4 u_InvProjMatrix;
uniform mat4 u_InvViewMatrix;

-- vs
layout(location = 0) in vec3 vs_Position;
out vec3 dir;
out vec3 relativeDir;

void main()
{
	vec3 WSD = u_SunDir;

    dir = (u_InvViewMatrix * vec4((u_InvProjMatrix * vec4(vs_Position, 1.0)).xyz, 0.0)).xyz;

    float theta = acos(WSD.z);
    float phi = atan(WSD.y, WSD.x);
    mat3 rz = mat3(cos(phi), -sin(phi), 0.0, sin(phi), cos(phi), 0.0, 0.0, 0.0, 1.0);
    mat3 ry = mat3(cos(theta), 0.0, sin(theta), 0.0, 1.0, 0.0, -sin(theta), 0.0, cos(theta));
	
    // apply this rotation to view dir to get relative viewdir
    relativeDir = (ry * rz) * dir;

    gl_Position = vec4(vs_Position.xy, 0.9999999, 1.0);
}

-- fs
layout(location = 0) out vec4 frag;
in vec3 dir;
in vec3 relativeDir;

void main()
{
	vec3 WSD = u_SunDir;
    vec3 WCP = u_CameraPos;

    vec3 d = normalize(dir);

    vec3 sunColor = outerSunRadiance(relativeDir);

    vec3 extinction;
    vec3 inscatter = skyRadiance(WCP, d, WSD, extinction, 0.0) * 0.15;
    vec3 finalColor = sunColor * extinction + inscatter;
	
    frag = vec4(finalColor, 1);
}