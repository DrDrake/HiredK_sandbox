#include "atmosphere/common.h"

#define MAX_NUM_SHADOW_SPLITS 4

uniform sampler2DArray s_SunDepthTex;
uniform sampler2D s_Coverage;

uniform float u_DepthSplit;
uniform bool u_EnableLogZ;
uniform float u_OceanLevel;

uniform vec2 u_CameraClip;
uniform vec3 u_CameraPosition;
uniform vec3 u_SunDirection;

uniform mat4 u_SunShadowMatrix[MAX_NUM_SHADOW_SPLITS];
uniform float u_SunShadowResolution;
uniform vec4 u_SunShadowSplits;

uniform float u_CoverageScale;
uniform vec2 u_CoverageOffset;
uniform float u_EarthRadius;
uniform float u_StartHeight;

float Fcoef(float z) {
	return 2.0 / log2(z + 1.0);
}

float textureCompare(sampler2DArray depths, float index, vec2 uv, float compare)
{
	float depth = texture(depths, vec3(uv, index)).r;
	return step(compare, depth);
}

float textureShadowLerp(sampler2DArray depths, float index, vec2 size, vec2 uv, float compare)
{
	vec2 texelSize = vec2(1.0) / size;
	vec2 f = fract(uv * size + 0.5);
	vec2 centroidUV = floor(uv * size + 0.5) / size;
	
	float lb = textureCompare(depths, index, centroidUV + texelSize * vec2(0.0, 0.0), compare);
	float lt = textureCompare(depths, index, centroidUV + texelSize * vec2(0.0, 1.0), compare);
	float rb = textureCompare(depths, index, centroidUV + texelSize * vec2(1.0, 0.0), compare);
	float rt = textureCompare(depths, index, centroidUV + texelSize * vec2(1.0, 1.0), compare);	
	float a = mix(lb, lt, f.y);
	float b = mix(rb, rt, f.y);
	return mix(a, b, f.x);
}

float PCF(sampler2DArray depths, float index, vec2 size, vec2 uv, float compare)
{
	float result = 0.0;	
	for(int x = -1; x <= 1; x++)
	for(int y = -1; y <= 1; y++) {
		result += textureShadowLerp(depths, index, size, uv + (vec2(x, y) / size), compare);
	}

	return result / 9.0;
}

vec3 InternalRaySphereIntersect(float radius, vec3 origin, vec3 direction)
{	
	float a0 = radius * radius - dot(origin, origin);
	float a1 = dot(origin, direction);
	float result = sqrt(a1 * a1 + a0) - a1;
	
	return origin + direction * result;
}

float GetCloudsShadowFactor(vec3 pos)
{
	vec3 intersect = InternalRaySphereIntersect(u_EarthRadius + u_StartHeight, pos, u_SunDirection);	
	vec2 unit = intersect.xz * u_CoverageScale;
	vec2 coverageUV = unit * 0.5 + 0.5;
	coverageUV += u_CoverageOffset;
	
	vec4 coverage = texture(s_Coverage, coverageUV);
	float cloudShadow = coverage.r;	
	cloudShadow *= 0.5;
	
	return 1.0 - cloudShadow;
}

float GetShadowFactor(vec3 pos, float depth)
{
	float factor = GetCloudsShadowFactor(pos + vec3(0, u_EarthRadius, 0)); // temp
	
	int index = 0;
	if (depth < u_SunShadowSplits.x) {
		index = 0;
	}
	else if (depth < u_SunShadowSplits.y) {
		index = 1;
	}
	else if (depth < u_SunShadowSplits.z) {
		index = 2;
	}
	else if (depth < u_SunShadowSplits.w) {
		index = 3;
	}
	else {
		return factor;
	}
	
	vec4 shadow_coord = u_SunShadowMatrix[index] * vec4(pos, 1);	
	factor *= PCF(s_SunDepthTex, index, vec2(u_SunShadowResolution), shadow_coord.xy, shadow_coord.z);
	return factor;
}