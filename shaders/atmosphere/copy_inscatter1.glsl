#version 400 core

/**
* Copies deltaS into S (line 5 in algorithm 4.1)
*/

#include "common_preprocess.h"

uniform sampler3D deltaSRSampler;
uniform sampler3D deltaSMSampler;

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
    vec3 uvw = vec3(gl_FragCoord.xy, float(layer) + 0.5) / vec3(ivec3(RES_MU_S * RES_NU, RES_MU, RES_R));
    vec4 ray = texture(deltaSRSampler, uvw);
    vec4 mie = texture(deltaSMSampler, uvw);
	
	// store only red component of single Mie scattering (cf. "Angular precision")
    frag = vec4(ray.rgb, mie.r);
}