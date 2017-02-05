#version 400 core

uniform sampler2D _ElevationSampler;
uniform sampler2D _NormalSampler;

-- vs
layout(location = 0) in vec2 vs_Position;
layout(location = 1) in vec2 vs_TexCoord;
out vec2 fs_TexCoord;

void main()
{
	gl_Position = vec4(vs_Position, 0, 1);
	fs_TexCoord = vs_TexCoord;
}

-- fs
layout(location = 0) out vec4 frag;
in vec2 fs_TexCoord;

const float SLOPE_LIMIT = 0.94f;

void main()
{
    float h = texture(_ElevationSampler, fs_TexCoord).x;
    
    vec3 fn;
	fn.xy = texture(_NormalSampler, fs_TexCoord).xy * 2.0 - 1.0;
	fn.z = sqrt(max(0.0, 1.0 - dot(fn.xy, fn.xy)));
    
    float slope = clamp(dot(normalize(fn), vec3(0, 0, 1)), 0, 1);

    vec4 splat; 
    splat.r = (h <= 6.0 && abs(slope) >= SLOPE_LIMIT) ? 1.0 : 0.0;
    splat.g = (h >  6.0 && abs(slope) >= SLOPE_LIMIT) ? 1.0 : 0.0;
    splat.b = (abs(slope) < SLOPE_LIMIT) ? 1.0 : 0.0;
    splat.a = 1.0;
    
	frag = splat;
}