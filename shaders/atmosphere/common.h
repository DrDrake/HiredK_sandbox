#include "atmosphere/common_preprocess.h"

const float SUN_INTENSITY = 100.0;

uniform sampler2D skyIrradianceSampler;
uniform sampler3D inscatterSampler;
uniform sampler2D glareSampler;

// ----------------------------------------------------------------------------
// COMMON FUNCTIONS:
// ----------------------------------------------------------------------------

vec3 outerSunRadiance(vec3 viewdir) {
    vec3 data = viewdir.z > 0.0 ? texture(glareSampler, vec2(0.5) + viewdir.xy * 4.0).rgb : vec3(0.0);
    return pow(data, vec3(2.2)) * SUN_INTENSITY;
}

vec3 sunRadiance(float r, float muS) {
	return transmittanceWithShadow(r, muS) * SUN_INTENSITY;
}

vec3 skyIrradiance(float r, float muS) {
	return irradiance(skyIrradianceSampler, r, muS) * SUN_INTENSITY;
}

vec3 skyRadiance(vec3 camera, vec3 viewdir, vec3 sundir, out vec3 extinction, float shaftWidth)
{
    vec3 result;
    camera += viewdir * max(shaftWidth, 0.0);
    float r = length(camera);
    float rMu = dot(camera, viewdir);
    float mu = rMu / r;
    float r0 = r;
    float mu0 = mu;

    float deltaSq = sqrt(rMu * rMu - r * r + Rt*Rt);
    float din = max(-rMu - deltaSq, 0.0);
    if (din > 0.0) {
        camera += din * viewdir;
        rMu += din;
        mu = rMu / Rt;
        r = Rt;
    }

    if (r <= Rt) {
        float nu = dot(viewdir, sundir);
        float muS = dot(camera, sundir) / r;

        vec4 inScatter = texture4D(inscatterSampler, r, rMu / r, muS, nu);
        if (shaftWidth > 0.0) {
            if (mu > 0.0) {
                inScatter *= min(transmittance(r0, mu0) / transmittance(r, mu), 1.0).rgbr;
            } else {
                inScatter *= min(transmittance(r, -mu) / transmittance(r0, -mu0), 1.0).rgbr;
            }
        }
        extinction = transmittance(r, mu);

        vec3 inScatterM = getMie(inScatter);
        float phase = phaseFunctionR(nu);
        float phaseM = phaseFunctionM(nu);
        result = inScatter.rgb * phase + inScatterM * phaseM;
    } else {
        result = vec3(0.0);
        extinction = vec3(1.0);
    }

    return result * SUN_INTENSITY;
}

void sunRadianceAndSkyIrradiance(vec3 worldP, vec3 worldN, vec3 worldS, out vec3 sunL, out vec3 skyE)
{
    float r = length(worldP);
    if (r < 0.9 * 6360000.0) {
        worldP.z += 6360000.0;
        r = length(worldP);
    }
	
    vec3 worldV = worldP / r; // vertical vector
    float muS = dot(worldV, worldS);

    float sunOcclusion = 1.0;// - sunShadow;
    sunL = sunRadiance(r, muS) * sunOcclusion;

    // ambient occlusion due only to slope, does not take self shadowing into account
    float skyOcclusion = (1.0 + dot(worldV, worldN)) * 0.5;
    // factor 2.0 : hack to increase sky contribution (numerical simulation of
    // "precompued atmospheric scattering" gives less luminance than in reality)
	
    skyE = vec3(skyOcclusion);
	//skyE = 2.0 * skyIrradiance(r, muS) * skyOcclusion;
}

vec3 inScattering(vec3 camera, vec3 point, vec3 sundir, out vec3 extinction, float shaftWidth)
{
    vec3 result;
    vec3 viewdir = point - camera;
    float d = length(viewdir);
    viewdir = viewdir / d;
    float r = length(camera);
    if (r < 0.9 * Rg) {
        camera.z += Rg;
        point.z += Rg;
        r = length(camera);
    }
	
    float rMu = dot(camera, viewdir);
    float mu = rMu / r;
    float r0 = r;
    float mu0 = mu;
    point -= viewdir * clamp(shaftWidth, 0.0, d);

    float deltaSq = sqrt(rMu * rMu - r * r + Rt*Rt);
    float din = max(-rMu - deltaSq, 0.0);
    if (din > 0.0 && din < d) {
        camera += din * viewdir;
        rMu += din;
        mu = rMu / Rt;
        r = Rt;
        d -= din;
    }

    if (r <= Rt) {
        float nu = dot(viewdir, sundir);
        float muS = dot(camera, sundir) / r;

        vec4 inScatter;

        if (r < Rg + 600.0) {
            // avoids imprecision problems in aerial perspective near ground
            float f = (Rg + 600.0) / r;
            r = r * f;
            rMu = rMu * f;
            point = point * f;
        }

        float r1 = length(point);
        float rMu1 = dot(point, viewdir);
        float mu1 = rMu1 / r1;
        float muS1 = dot(point, sundir) / r1;

        if (mu > 0.0) {
            extinction = min(transmittance(r, mu) / transmittance(r1, mu1), 1.0);
        } else {
            extinction = min(transmittance(r1, -mu1) / transmittance(r, -mu), 1.0);
        }

        const float EPS = 0.004;
        float lim = -sqrt(1.0 - (Rg / r) * (Rg / r));
        if (abs(mu - lim) < EPS) {
            float a = ((mu - lim) + EPS) / (2.0 * EPS);

            mu = lim - EPS;
            r1 = sqrt(r * r + d * d + 2.0 * r * d * mu);
            mu1 = (r * mu + d) / r1;
            vec4 inScatter0 = texture4D(inscatterSampler, r, mu, muS, nu);
            vec4 inScatter1 = texture4D(inscatterSampler, r1, mu1, muS1, nu);
            vec4 inScatterA = max(inScatter0 - inScatter1 * extinction.rgbr, 0.0);

            mu = lim + EPS;
            r1 = sqrt(r * r + d * d + 2.0 * r * d * mu);
            mu1 = (r * mu + d) / r1;
            inScatter0 = texture4D(inscatterSampler, r, mu, muS, nu);
            inScatter1 = texture4D(inscatterSampler, r1, mu1, muS1, nu);
            vec4 inScatterB = max(inScatter0 - inScatter1 * extinction.rgbr, 0.0);

            inScatter = mix(inScatterA, inScatterB, a);
        } else {
            vec4 inScatter0 = texture4D(inscatterSampler, r, mu, muS, nu);
            vec4 inScatter1 = texture4D(inscatterSampler, r1, mu1, muS1, nu);
            inScatter = max(inScatter0 - inScatter1 * extinction.rgbr, 0.0);
        }
		
        // avoids imprecision problems in Mie scattering when sun is below horizon
        inScatter.w *= smoothstep(0.00, 0.02, muS);

        vec3 inScatterM = getMie(inScatter);
        float phase = phaseFunctionR(nu);
        float phaseM = phaseFunctionM(nu);
        result = inScatter.rgb * phase + inScatterM * phaseM;
    } else {
        result = vec3(0.0);
        extinction = vec3(1.0);
    }

    return result * SUN_INTENSITY;
}