#version 400 core

#include "atmosphere/common.h"
const vec3 OceanColor = vec3(0, 0, 1);

uniform mat4 u_Deform_ScreenQuadCorners;
uniform mat4 u_Deform_ScreenQuadVerticals;
uniform vec4 u_Deform_ScreenQuadCornerNorms;
uniform vec4 u_Deform_Offset;
uniform float u_Deform_Radius;
uniform mat4 u_Deform_TangentFrameToWorld;
uniform mat3 u_Deform_LocalToWorld;
uniform vec2 u_Deform_Blending;
uniform vec4 u_Deform_Camera;

uniform vec3 u_HeightNormal_TileSize;
uniform vec3 u_HeightNormal_TileCoords;
uniform sampler2D s_HeightNormal_Slot0;
uniform sampler2D s_HeightNormal_Slot1;

uniform float u_OceanLevel;
uniform bool u_DrawOcean;

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
out vec3 p;

void main()
{
	vec2 zfc = texTile(s_HeightNormal_Slot0, vs_TexCoord, u_HeightNormal_TileCoords, u_HeightNormal_TileSize).xy;
	
	if	(zfc.x <= u_OceanLevel && u_DrawOcean) {
		zfc = vec2(0, 0);
	}
	
	vec2 vert = abs(u_Deform_Camera.xy - vs_Position.xy);
	float d = max(max(vert.x, vert.y), u_Deform_Camera.z);
	float blend = clamp((d - u_Deform_Blending.x) / u_Deform_Blending.y, 0.0, 1.0);
	
	float R = u_Deform_Radius;
	mat4 C = u_Deform_ScreenQuadCorners;
	mat4 N = u_Deform_ScreenQuadVerticals;
	vec4 L = u_Deform_ScreenQuadCornerNorms;
	vec3 P = vec3(vs_Position.xy * u_Deform_Offset.z + u_Deform_Offset.xy, R);
	
	vec4 uvUV = vec4(vs_Position.xy, vec2(1.0, 1.0) - vs_Position.xy);
	vec4 alpha = uvUV.zxzx * uvUV.wwyy;
	vec4 alphaPrime = alpha * L / dot(alpha, L);
	
	float h = zfc.x * (1.0 - blend) + zfc.y * blend;
	float k = min(length(P) / dot(alpha, L) * 1.0000003, 1.0);
	float hPrime = (h + R * (1.0 - k)) / k;	
	
	p = (u_Deform_Radius + h) * normalize(u_Deform_LocalToWorld * P);
	
	gl_Position = (C + hPrime * N) * alphaPrime;
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
	fn.xy = texTile(s_HeightNormal_Slot1, fs_TexCoord, u_HeightNormal_TileCoords, u_HeightNormal_TileSize).xy;
	fn.z = sqrt(max(0.0, 1.0 - dot(fn.xy, fn.xy)));	
	if	(ht <= u_OceanLevel && u_DrawOcean) {
		fn = vec3(0, 0, 1);		
	}
	
	mat3 TTW = mat3(u_Deform_TangentFrameToWorld);
    fn = TTW * fn;
	
	vec3 color = vec3(0, 1, 0);
	float roughness = 0.0f;
	
	if	(ht <= u_OceanLevel && u_DrawOcean) {
		color = OceanColor;
		roughness = 0.002f;
	}
	
	if (gl_FragCoord.z >= 0.99998) {
		color = surfaceLighting(p, color, u_CameraPos, u_SunDir, fn, roughness, 1.0);
	}
	
	diffuse = vec4(color, 1);
	geometric = vec4(fn, roughness);
}