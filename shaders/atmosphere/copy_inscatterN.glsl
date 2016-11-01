#version 400 core

/**
* Adds deltaS into S (line 11 in algorithm 4.1)
*/

#include "common_preprocess.h"

uniform sampler3D deltaSSampler;

uniform float r;
uniform vec4 dhdH;
uniform int layer;

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
    vec3 uvw = vec3(gl_FragCoord.xy, float(layer) + 0.5) / vec3(ivec3(RES_MU_S * RES_NU, RES_MU, RES_R));
    frag = vec4(texture(deltaSSampler, uvw).rgb / phaseFunctionR(nu), 0.0);
}