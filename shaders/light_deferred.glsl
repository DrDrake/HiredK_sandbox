#version 400 core

#include "common.h"

uniform mat4 u_InvViewProjMatrix;
uniform bool u_EnableShadow;

uniform sampler2D s_Tex0;
uniform sampler2D s_Tex1;
uniform sampler2D s_Tex2;
uniform sampler2D s_Tex3;

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

void main()
{
    float depth = texture(s_Tex0, fs_TexCoord).r;
    vec4 diffuse = texture(s_Tex1, fs_TexCoord);
    vec4 geometric = texture(s_Tex2, fs_TexCoord);  
    vec3 color = diffuse.rgb;
    
    if (!u_EnableLogZ && depth < u_DepthSplit)
    {
        vec3 p = GetWorldPos(fs_TexCoord, depth);      
        vec3 n = normalize(geometric.xyz * 2.0 - 1.0);
        
        float ao = 1.0 - texture(s_Tex3, fs_TexCoord).r;
        float shadow = 1.0;
        
        if (u_EnableShadow) {
            shadow = GetShadowFactor(p, depth);
        }
        
        vec3 I = p - u_SectorCameraPos;
        vec3 R = reflect(I, n);      
        vec3 reflect = texture(s_Cubemap, R).rgb;
        
        color = mix(color, reflect, geometric.a);      
        color = surfaceLighting(p + vec3(0, 6360000.0, 0), color, u_CameraPos, u_SunDir, n, 0.0, shadow);
        color *= ao;
    }
    
	frag = vec4(color, diffuse.a);
}