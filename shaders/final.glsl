#version 400 core

const int MAX_MOTION_SAMPLES = 64;

uniform mat4 u_InvViewProjMatrix;
uniform mat4 u_PreViewProjMatrix;
uniform float u_MotionScale;

uniform sampler2D s_Tex0;
uniform sampler2D s_Tex1;

uniform bool u_EnableLensflare;
uniform mat4 u_LensStarMatrix;
uniform sampler2D s_Tex2;
uniform sampler2D s_Tex3;
uniform sampler2D s_Tex4;

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

vec4 motion_blur()
{
	float depth = texture(s_Tex0, fs_TexCoord).r;
	vec4 cpos = vec4(fs_TexCoord * 2.0 - 1.0, depth, 1.0);
	cpos = u_InvViewProjMatrix * cpos;
	vec4 ppos = u_PreViewProjMatrix * cpos;
	ppos.xyz /= ppos.w;
	ppos.xy = ppos.xy * 0.5 + 0.5;
	
	vec2 velocity = (ppos.xy - fs_TexCoord) * u_MotionScale;
	vec2 texelSize = 1.0 / vec2(textureSize(s_Tex1, 0));
	
	float tspeed = length(velocity / texelSize); // speed in pixels/second
	int nSamples = clamp(int(tspeed), 1, MAX_MOTION_SAMPLES);

	vec4 result = texture(s_Tex1, fs_TexCoord);
	for (int i = 1; i < nSamples; ++i) {
		vec2 offset = fs_TexCoord + velocity * (float(i) / float(nSamples - 1) - 0.5);
		result += texture(s_Tex1, offset);
	}
	
	return result / float(nSamples);
}

void main()
{
	frag = motion_blur();
    
    if (u_EnableLensflare)
    {
        vec4 lensMod = texture(s_Tex3, fs_TexCoord);
        vec2 lensStarTexcoord = (u_LensStarMatrix * vec4(fs_TexCoord, 0.0, 1.0)).xy;
        lensMod += texture(s_Tex4, lensStarTexcoord);
        
        frag += (texture(s_Tex2, fs_TexCoord) * lensMod * 0.25);
    }
}