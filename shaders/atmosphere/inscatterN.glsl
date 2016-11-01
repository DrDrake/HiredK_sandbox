#version 400 core

/**
* Computes higher order scattering (line 9 in algorithm 4.1)
*/

#include "common_preprocess.h"

uniform sampler3D deltaJSampler;

uniform float r;
uniform vec4 dhdH;
uniform int layer;

vec3 integrand(float r, float mu, float muS, float nu, float t) {
    float ri = sqrt(r * r + t * t + 2.0 * r * mu * t);
    float mui = (r * mu + t) / ri;
    float muSi = (nu * t + muS * r) / ri;
    return texture4D(deltaJSampler, ri, mui, muSi, nu).rgb * transmittance(r, mu, t);
}

vec3 inscatter(float r, float mu, float muS, float nu) {
    vec3 raymie = vec3(0.0);
    float dx = limit(r, mu) / float(INSCATTER_INTEGRAL_SAMPLES);
    float xi = 0.0;
    vec3 raymiei = integrand(r, mu, muS, nu, 0.0);
    for (int i = 1; i <= INSCATTER_INTEGRAL_SAMPLES; ++i) {
        float xj = float(i) * dx;
        vec3 raymiej = integrand(r, mu, muS, nu, xj);
        raymie += (raymiei + raymiej) / 2.0 * dx;
        xi = xj;
        raymiei = raymiej;
    }
	
    return raymie;
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
layout(location = 0) out vec4 frag;

void main() {
    float mu, muS, nu;
    getMuMuSNu(r, dhdH, mu, muS, nu);
    frag.rgb = inscatter(r, mu, muS, nu);
}