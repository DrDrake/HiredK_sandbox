#version 400 core

#include "common.h"
#include "common_planet.h"

const vec3 OceanColor = vec3(0.039f, 0.156f, 0.47f);

uniform float u_EarthRadius;
uniform float u_StartHeight;
uniform vec2 u_CoverageOffset;
uniform float u_CoverageScale;
uniform sampler2D s_Coverage;

-- vs
layout(location = 0) in vec3 vs_Position;
layout(location = 1) in vec2 vs_TexCoord;
out vec2 tcs_Position;
out vec2 tcs_TexCoord;
out vec3 tcs_Deformed;

void main()
{ 
    vec3 deformed;
	if (u_Spherical) {
		gl_Position = getSphericalPos(vs_Position.xy, vs_TexCoord, deformed);
	}
	else {
		gl_Position = getPos(vs_Position.xy, vs_TexCoord, deformed);
	}
	
    tcs_Position = vs_Position.xy;
    tcs_TexCoord = vs_TexCoord;
	tcs_Deformed = deformed;
}

-- tcs
layout(vertices = 4) out;
in vec2 tcs_Position[];
in vec2 tcs_TexCoord[];
in vec3 tcs_Deformed[];

out vec2 tes_Position[];
out vec2 tes_TexCoord[];

float level(float d) {
	return clamp(1.5 * 2000 / d, 1, 25);
}

void main()
{
	if(gl_InvocationID == 0)
	{
        float d0 = distance(u_WorldCamPosition, tcs_Deformed[0].xyz);
        float d1 = distance(u_WorldCamPosition, tcs_Deformed[1].xyz);
        float d2 = distance(u_WorldCamPosition, tcs_Deformed[2].xyz);
        float d3 = distance(u_WorldCamPosition, tcs_Deformed[3].xyz);
        
        gl_TessLevelOuter[0] = level(mix(d3, d0, 0.5));
        gl_TessLevelOuter[1] = level(mix(d0, d1, 0.5));
        gl_TessLevelOuter[2] = level(mix(d1, d2, 0.5));
        gl_TessLevelOuter[3] = level(mix(d2, d3, 0.5));
        
        float l = max(max(gl_TessLevelOuter[0], gl_TessLevelOuter[1]), max(gl_TessLevelOuter[2], gl_TessLevelOuter[3]));
        gl_TessLevelInner[0] = l;
        gl_TessLevelInner[1] = l;
    }
    
    tes_Position[gl_InvocationID] = tcs_Position[gl_InvocationID];
    tes_TexCoord[gl_InvocationID] = tcs_TexCoord[gl_InvocationID];
}

-- tes
layout(quads, fractional_odd_spacing, ccw) in;

in vec2 tes_Position[];
in vec2 tes_TexCoord[];

out vec2 fs_TexCoord;
out vec3 fs_Deformed;

vec2 interpolate(vec2 bl, vec2 br, vec2 tr, vec2 tl)
{
	float u = gl_TessCoord.x;
	float v = gl_TessCoord.y;
	
	vec2 b = mix(bl, br, u);
	vec2 t = mix(tl, tr, u);	
	return mix(b, t, v);
}

void main()
{
    vec2 Position = interpolate(tes_Position[0], tes_Position[1], tes_Position[2], tes_Position[3]);
    vec2 TexCoord = interpolate(tes_TexCoord[0], tes_TexCoord[1], tes_TexCoord[2], tes_TexCoord[3]);
    
    vec3 deformed;	
	if (u_Spherical) {
		gl_Position = getSphericalPos(Position, TexCoord, deformed);
	}
	else {
		gl_Position = getPos(Position, TexCoord, deformed);
	}
	
    fs_TexCoord = TexCoord;
    fs_Deformed = deformed;
}

-- fs
layout(location = 0) out vec4 albedo;
layout(location = 1) out vec4 normal;
in vec2 fs_TexCoord;
in vec3 fs_Deformed;

void main()
{
	float ht = texTile(s_HeightNormal_Slot0, fs_TexCoord, u_HeightNormal_TileCoords, u_HeightNormal_TileSize).x;
	vec3 N;
	
	if	(!u_Spherical && ht < 0.0) {
		mat3 TTW = mat3(u_Deform_TangentFrameToWorld);
		N = normalize(TTW * vec3(0, 0, 1));
		albedo = vec4(OceanColor, 1);
	}
	else {	
		albedo = ProceduralShading(fs_TexCoord, N);
	}
	
	if (gl_FragCoord.z >= u_DeferredDist) {
		vec3 L = normalize(u_SunDirection);		
		float offset = (u_Spherical) ? 0.0 : 6360000.0;
		
		// todo better
		vec3 offset2 = fs_Deformed - vec3(0, u_Deform_Radius, 0);
		float shadow = GetCloudsShadowFactor(s_Coverage, u_EarthRadius + u_StartHeight, offset2 + vec3(0, u_EarthRadius, 0), L, u_CoverageOffset, u_CoverageScale);   		
		albedo.rgb = BRDF_Atmospheric(fs_Deformed + vec3(0, offset, 0), N, albedo.rgb, u_WorldCamPosition + vec3(0, offset, 0), L, shadow);
	}
	
	if (u_DrawQuadtree) {
		albedo.r += mod(dot(floor(u_Deform_Offset.xy / u_Deform_Offset.z + 0.5), vec2(1.0)), 2.0) * 0.1;
	}
	
	if (u_DrawPencil) {
		if (length(fs_Deformed.xz - u_Pencil.xz) < u_Pencil.w) {
			albedo.rgb = u_PencilColor.rgb;
		}
	}
	
	normal = vec4(N * 0.5 + 0.5, 0);
}