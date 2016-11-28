#version 400 core

#include "common.h"

uniform mat4 u_Deform_ScreenQuadCorners;
uniform mat4 u_Deform_ScreenQuadVerticals;
uniform vec4 u_Deform_Offset;
uniform vec2 u_Deform_Blending;
uniform vec4 u_Deform_Camera;

uniform vec3 u_HeightNormal_TileSize;
uniform vec3 u_HeightNormal_TileCoords;
uniform sampler2D s_HeightNormal_Slot0;
uniform sampler2D s_HeightNormal_Slot1;

vec4 texTile(sampler2D tile, vec2 uv, vec3 tileCoords, vec3 tileSize) {
	uv = tileCoords.xy + uv * tileSize.xy;
	return texture2D(tile, uv);
}

-- vs
layout(location = 0) in vec3 vs_Position;
layout(location = 1) in vec3 vs_Normal;
layout(location = 2) in vec2 vs_TexCoord;
out vec3 fs_Normal;
out vec2 fs_TexCoord;
out float fs_Flogz;
out vec3 p;

void main()
{
	vec2 zfc = texTile(s_HeightNormal_Slot0, vs_TexCoord, u_HeightNormal_TileCoords, u_HeightNormal_TileSize).xy;
    
	if	(zfc.x <= u_OceanLevel) {
		zfc = vec2(0, 0);
	}
	
	vec2 vert = abs(u_Deform_Camera.xy - vs_Position.xy);   
	float d = max(max(vert.x, vert.y), u_Deform_Camera.z);   
	float blend = clamp((d - u_Deform_Blending.x) / u_Deform_Blending.y, 0.0, 1.0);
	
	float h = zfc.x * (1.0 - blend) + zfc.y * blend;   
    p = vec3(vs_Position.xy * u_Deform_Offset.z + u_Deform_Offset.xy, h);
    
    mat4 C = u_Deform_ScreenQuadCorners;
    mat4 N = u_Deform_ScreenQuadVerticals;
    
    vec4 uvUV = vec4(vs_Position.xy, vec2(1.0) - vs_Position.xy);
    vec4 alpha = uvUV.zxzx * uvUV.wwyy;
    gl_Position = (C + h * N) * alpha;
    
	fs_Normal = vs_Normal;
	fs_TexCoord = vs_TexCoord;
}

-- fs
layout(location = 0) out vec4 diffuse;
layout(location = 1) out vec4 geometric;
in vec3 fs_Normal;
in vec2 fs_TexCoord;
in float fs_Flogz;
in vec3 p;

void main()
{
	float ht = texTile(s_HeightNormal_Slot0, fs_TexCoord, u_HeightNormal_TileCoords, u_HeightNormal_TileSize).x;
	
	vec3 fn;
	fn.xz = texTile(s_HeightNormal_Slot1, fs_TexCoord, u_HeightNormal_TileCoords, u_HeightNormal_TileSize).xy * 2.0 - 1.0;
	fn.y = sqrt(max(0.0, 1.0 - dot(fn.xz, fn.xz)));	
    
	if (ht <= u_OceanLevel) {
		fn = vec3(0, 1, 0);		
	}
	
	vec3 color = vec3(0, 1, 0) * 0.02;
	float roughness = 0.0f;
    
    if (ht <= u_OceanLevel) {
        color = vec3(0, 0, 1) * 0.02;
        roughness = 0.1f;
    }
    
	diffuse = vec4(color, 1);
	geometric = vec4(fn * 0.5 + 0.5, roughness);
}