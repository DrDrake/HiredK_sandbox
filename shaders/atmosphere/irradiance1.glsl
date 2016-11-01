#version 400 core

/**
* Computes ground irradiance due to direct sunlight E[L0]
* (line 2 in algorithm 4.1)
*/

#include "common_preprocess.h"

-- vs
layout(location = 0) in vec4 vs_Position;

void main() {
    gl_Position = vs_Position;
}

-- fs
layout(location = 0) out vec4 frag;

void main() {
    float r, muS;
    getIrradianceRMuS(r, muS);
    frag = vec4(transmittance(r, muS) * max(muS, 0.0), 0.0);
}