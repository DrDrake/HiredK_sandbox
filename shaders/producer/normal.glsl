#version 400 core

uniform sampler2D _ElevationSampler;

uniform vec2 _TileSD;
uniform vec4 _ElevationOSL;

uniform mat4 _PatchCorners;
uniform mat4 _PatchVerticals;
uniform vec4 _PatchCornerNorms;
uniform mat4 _WorldToTangentFrame;
uniform vec4 _Deform;

-- vs
layout(location = 0) in vec4 vs_Position;
out vec2 fs_UV;
out vec2 fs_ST;

void main() {	
	gl_Position = vs_Position;
	fs_UV = (vs_Position.xy * 0.5+0.5);
	fs_ST = (vs_Position.xy * 0.5+0.5) * _TileSD.x;
}

-- fs
in vec2 fs_UV;
in vec2 fs_ST;
out vec4 frag;

vec3 GetWorldPosition(vec2 uv, float h) 
{
	uv = uv / (_TileSD.x - 1.0);
	
	float R = _Deform.w;
	mat4 C = _PatchCorners;
	mat4 N = _PatchVerticals;
	vec4 L = _PatchCornerNorms;
	
	vec4 uvUV = vec4(uv, vec2(1.0,1.0) - uv);
	vec4 alpha = uvUV.zxzx * uvUV.wwyy;
	vec4 alphaPrime = alpha * L / dot(alpha, L);
	
	vec4 up = N * alphaPrime;
	float k = mix(length(up.xyz), 1.0, smoothstep(R / 32.0, R / 64.0, _Deform.z));
	float hPrime = (h + R * (1.0 - k)) / k;
	
	return ((C * alphaPrime) + hPrime * up).xyz;
}

void main() {
	vec2 p_uv = floor(fs_ST);
	
	vec4 uv0 = floor(p_uv.xyxy + vec4(-1.0,0.0,1.0,0.0)) * _ElevationOSL.z + _ElevationOSL.xyxy;
	vec4 uv1 = floor(p_uv.xyxy + vec4(0.0,-1.0,0.0,1.0)) * _ElevationOSL.z + _ElevationOSL.xyxy;

	float z0 = textureLod(_ElevationSampler, uv0.xy, 0).x;
	float z1 = textureLod(_ElevationSampler, uv0.zw, 0).x;
	float z2 = textureLod(_ElevationSampler, uv1.xy, 0).x;
	float z3 = textureLod(_ElevationSampler, uv1.zw, 0).x;
	
	vec3 p0 = GetWorldPosition(p_uv + vec2(-1.0, 0.0), z0).xyz;
	vec3 p1 = GetWorldPosition(p_uv + vec2(+1.0, 0.0), z1).xyz;
	vec3 p2 = GetWorldPosition(p_uv + vec2(0.0, -1.0), z2).xyz;
	vec3 p3 = GetWorldPosition(p_uv + vec2(0.0, +1.0), z3).xyz;
	
	mat3 worldToTangentFrame = mat3(_WorldToTangentFrame);
	vec2 nf = (worldToTangentFrame * normalize(cross(p1 - p0, p3 - p2))).xy;
    
	frag = vec4(nf * 0.5 + 0.5, 0.0, 1.0);
}