#version 400 core

/**
* Copies deltaE into E (lines 4 and 10 in algorithm 4.1)
*/

#include "common_preprocess.h"

uniform sampler2D deltaESampler;
uniform float k;

-- vs
layout(location = 0) in vec4 vs_Position;

void main() {
    gl_Position = vs_Position;
}

-- fs
layout(location = 0) out vec4 frag;

void main() {
    vec2 uv = gl_FragCoord.xy / vec2(SKY_W, SKY_H);
    frag = k * texture(deltaESampler, uv);
}