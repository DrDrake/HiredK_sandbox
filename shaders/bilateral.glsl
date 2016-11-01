#version 400 core

#define RESOLUTION vec2(1280.0, 720.0)

uniform vec2 FullRes = vec2(RESOLUTION.x, RESOLUTION.y);
uniform vec2 InvFullRes = 1.0 / vec2(RESOLUTION.x, RESOLUTION.y);

uniform sampler2D s_Tex0;

uniform vec2 u_BlurDirection;

#define KERNEL_RADIUS 8.0
uniform vec2 u_LinMAD;

-- vs
layout(location = 0) in vec3 vs_Position;
layout(location = 1) in vec2 vs_TexCoord;
out vec2 fs_TexCoord;

void main()
{
	gl_Position = vec4(vs_Position, 1);
	fs_TexCoord = vs_TexCoord;
}

-- fs
layout(location = 0) out vec2 frag;
in vec2 fs_TexCoord;

vec2 SampleAOZ(vec2 uv)
{
    return texture(s_Tex0, fs_TexCoord + uv * InvFullRes).rg;
}

vec2 PointSampleAOZ(vec2 uv)
{
	ivec2 coord = ivec2(round(gl_FragCoord.xy + uv));
	return texelFetch(s_Tex0, coord, 0).rg;
}

float CrossBilateralWeight(float r, float z, float z0)
{
	const float BlurSigma = (KERNEL_RADIUS+1.0f) * 0.5f;
	const float BlurFalloff = 1.0f / (2.0f*BlurSigma*BlurSigma);

	float dz = z0 - z;
	return exp2(-r*r*BlurFalloff - dz*dz);
}

void main()
{
	vec2 aoz = SampleAOZ(vec2(0));
	float center_z = aoz.y;

	float w = 1.0;
	float total_ao = aoz.x * w;
	float total_weight = w;
	float i = 1.0;

	for(; i <= KERNEL_RADIUS/2; i += 1.0)
	{
		aoz = SampleAOZ( vec2(i) * u_BlurDirection );
		w = CrossBilateralWeight(i, aoz.y, center_z);
		total_ao += aoz.x * w;
		total_weight += w;

		aoz = SampleAOZ( vec2(-i) * u_BlurDirection );
		w = CrossBilateralWeight(i, aoz.y, center_z);
		total_ao += aoz.x * w;
		total_weight += w;
	}

	for(; i <= KERNEL_RADIUS; i += 2.0)
	{
		aoz = SampleAOZ( vec2(0.5+i) * u_BlurDirection );
		w = CrossBilateralWeight(i, aoz.y, center_z);
		total_ao += aoz.x * w;
		total_weight += w;

		aoz = SampleAOZ( vec2(-0.5-i) * u_BlurDirection );
		w = CrossBilateralWeight(i, aoz.y, center_z);
		total_ao += aoz.x * w;
		total_weight += w;
	}

	float ao = total_ao / total_weight;
	frag = vec2(ao, center_z);
}