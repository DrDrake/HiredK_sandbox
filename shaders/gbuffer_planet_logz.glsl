#version 400 core

#include "common.h"
#include "common_planet.h"

const vec3 OceanColor = vec3(0.039f, 0.156f, 0.47f);

float Fcoef(float z) {
	return 2.0 / log2(z + 1.0);
}

-- vs
layout(location = 0) in vec3 vs_Position;
layout(location = 1) in vec2 vs_TexCoord;
out vec2 fs_TexCoord;
out vec3 fs_Deformed;
out float fs_Flogz;

void main()
{
    vec3 deformed;
	gl_Position = getSphericalPos(vs_Position.xy, vs_TexCoord, deformed);
	gl_Position.z = log2(max(1e-6, 1.0 + gl_Position.w)) * Fcoef(u_CameraClip.y);
	fs_Flogz = 1.0 + gl_Position.w;
	
    fs_TexCoord = vs_TexCoord;
	fs_Deformed = deformed;
}

-- fs
layout(location = 0) out vec4 albedo;
layout(location = 1) out vec4 normal;
in vec2 fs_TexCoord;
in vec3 fs_Deformed;
in float fs_Flogz;

void main()
{
	float ht = texTile(s_HeightNormal_Slot0, fs_TexCoord, u_HeightNormal_TileCoords, u_HeightNormal_TileSize).x;
	vec3 N;
	
	if	(ht < 0.0) {
		mat3 TTW = mat3(u_Deform_TangentFrameToWorld);
		N = normalize(TTW * vec3(0, 0, 1));
		albedo = vec4(OceanColor, 1);
	}
	else {	
		albedo = ProceduralShading(fs_TexCoord, N);
	}
	
	vec3 L = normalize(u_SunDirection);
	albedo.rgb = BRDF_Atmospheric(fs_Deformed, N, albedo.rgb, u_WorldCamPosition, L, 1.0);
	normal = vec4(N * 0.5 + 0.5, 0);
	
	gl_FragDepth = log2(fs_Flogz) * 0.5 * Fcoef(u_CameraClip.y);
}