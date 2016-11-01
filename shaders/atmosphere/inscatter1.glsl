#version 400 core

/**
* Computes single scattering (line 3 in algorithm 4.1)
*/

#include "common_preprocess.h"

uniform float r;
uniform vec4 dhdH;
uniform int layer;

void integrand(float r, float mu, float muS, float nu, float t, out vec3 ray, out vec3 mie)
{
    ray = vec3(0.0);
    mie = vec3(0.0);
    float ri = sqrt(r * r + t * t + 2.0 * r * mu * t);
    float muSi = (nu * t + muS * r) / ri;
    ri = max(Rg, ri);
    if (muSi >= -sqrt(1.0 - Rg * Rg / (ri * ri))) {
        vec3 ti = transmittance(r, mu, t) * transmittance(ri, muSi);
        ray = exp(-(ri - Rg) / HR) * ti;
        mie = exp(-(ri - Rg) / HM) * ti;
    }
}

void inscatter(float r, float mu, float muS, float nu, out vec3 ray, out vec3 mie)
{
    ray = vec3(0.0);
    mie = vec3(0.0);
    float dx = limit(r, mu) / float(INSCATTER_INTEGRAL_SAMPLES);
    float xi = 0.0;
    vec3 rayi;
    vec3 miei;
    integrand(r, mu, muS, nu, 0.0, rayi, miei);
    for (int i = 1; i <= INSCATTER_INTEGRAL_SAMPLES; ++i) {
        float xj = float(i) * dx;
        vec3 rayj;
        vec3 miej;
        integrand(r, mu, muS, nu, xj, rayj, miej);
        ray += (rayi + rayj) / 2.0 * dx;
        mie += (miei + miej) / 2.0 * dx;
        xi = xj;
        rayi = rayj;
        miei = miej;
    }
	
    ray *= betaR;
    mie *= betaMSca;
}

-- vs
layout(location = 0) in vec4 vs_Position;

void main() {
    gl_Position = vs_Position;
}

-- gs
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

void main() {
    gl_Position = gl_in[0].gl_Position;
    gl_Layer = layer;
    EmitVertex();
	
    gl_Position = gl_in[1].gl_Position;	
    gl_Layer = layer;
    EmitVertex();
	
    gl_Position = gl_in[2].gl_Position;	
    gl_Layer = layer;
    EmitVertex();
	
    EndPrimitive();
}

-- fs
layout(location = 0) out vec4 data0;
layout(location = 1) out vec4 data1;

void main()
{
    vec3 ray;
    vec3 mie;
    float mu, muS, nu;
    getMuMuSNu(r, dhdH, mu, muS, nu);
    inscatter(r, mu, muS, nu, ray, mie);
	
    // store separately Rayleigh and Mie contributions, WITHOUT the phase function factor
    // (cf "Angular precision")
    data0.rgb = ray;
    data1.rgb = mie;
}