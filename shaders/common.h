#include "atmosphere/common.h"

#define PI 3.1415926535897932
#define ONE_OVER_PI	0.318309

vec3 BRDF_Atmospheric(vec3 pos, vec3 n, vec3 color, vec3 camera, vec3 sun, float shadow)
{
	vec3 V = normalize(pos);
	vec3 P = V * max(length(pos), 6360000.0 + 10.0);
    
	vec3 sunL;
	vec3 skyE;
	sunRadianceAndSkyIrradiance(P, n, sun, sunL, skyE);	
	float cTheta = dot(n, sun) * shadow;

	vec3 result = color * 0.05 * (sunL * max(cTheta, 0.1) + skyE) / M_PI;
	
	vec3 extinction;
	vec3 inscatter = inScattering(camera, P, sun, extinction, 0.0) * 0.08;
	return result * extinction + inscatter;
}

vec3 F_Schlick(vec3 f0, float u)
{
	return f0 + (vec3(1.0) - f0) * pow(1.0 - u, 5.0);
}

float Vis_Schlick(float ndotl, float ndotv, float roughness)
{
	float a = roughness + 1.0;
	float k = a * a * 0.125;

	float Vis_SchlickV = ndotv * (1 - k) + k;
	float Vis_SchlickL = ndotl * (1 - k) + k;

	return 0.25 / (Vis_SchlickV * Vis_SchlickL);
}

float D_GGX(float ndoth, float roughness)
{
	float m = roughness * roughness;
	float m2 = m * m;
	float d = (ndoth * m2 - ndoth) * ndoth + 1.0;

	return m2 / max(PI * d * d, 1e-8);
}

vec3 BRDF_Lambertian(vec3 albedo, float metalness)
{
	vec3 color = mix(albedo, vec3(0.0), metalness);
	color *= ONE_OVER_PI;
	return color;
}

vec3 BRDF_CookTorrance(float ldoth, float ndoth, float ndotv, float ndotl, float roughness, float metalness)
{
	vec3 F0 = mix(vec3(0.04), vec3(1), metalness);
	vec3 F = F_Schlick(F0, ldoth);

	float Vis = Vis_Schlick(ndotl, ndotv, roughness);
	float D = D_GGX(ndoth, roughness);

	return F * Vis * D;
}

float GetCloudsShadowFactor(sampler2D tex, float radius, vec3 origin, vec3 direction, vec2 offset, float scale)
{
    float a0 = radius * radius - dot(origin, origin);
	float a1 = dot(origin, direction);
	float result = sqrt(a1 * a1 + a0) - a1;
    
	vec3 intersect = origin + direction * result;	
	vec2 unit = intersect.xz * scale;
	vec2 coverageUV = unit * 0.5 + 0.5;
	coverageUV += offset;
	
	vec4 coverage = texture(tex, coverageUV);
	float cloudShadow = coverage.r;	
	cloudShadow *= 0.5;
	
	return 1.0 - cloudShadow;
}