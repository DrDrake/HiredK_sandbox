#version 400 core

#define RESOLUTION vec2(1280.0, 720)

#define SMAA_PIXEL_SIZE vec2(1.0 / RESOLUTION.x, 1.0 / RESOLUTION.y)
#define SMAA_PRESET_ULTRA 1
#define SMAA_GLSL_4 1

uniform sampler2D s_Tex0;

-- vs
#define SMAA_ONLY_COMPILE_VS 1
#include "SMAA.h"

layout(location = 0) in vec3 vs_Position;
layout(location = 1) in vec2 vs_TexCoord;
out vec2 texcoord;
out vec4 offset[3];
out vec4 dummy2;

void main()
{
	texcoord = vs_TexCoord;
	
	vec4 dummy1 = vec4(0);
	SMAAEdgeDetectionVS(dummy1, dummy2, texcoord, offset);	
	gl_Position = vec4(vs_Position, 1);
}

-- fs
#define SMAA_ONLY_COMPILE_FS 1
#include "SMAA.h"

layout(location = 0) out vec4 frag;
in vec2 texcoord;
in vec4 offset[3];
in vec4 dummy2;

void main()
{
	frag = SMAAColorEdgeDetectionPS(texcoord, offset, s_Tex0);
}