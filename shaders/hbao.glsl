#version 400 core

#define RESOLUTION vec2(1280.0, 720.0)

uniform vec2 AORes = vec2(RESOLUTION.x, RESOLUTION.y);
uniform vec2 InvAORes = vec2(1.0/RESOLUTION.x, 1.0/RESOLUTION.y);
uniform vec2 NoiseScale = vec2(RESOLUTION.x, RESOLUTION.y) / 4.0;

uniform vec2 u_UVToViewA;
uniform vec2 u_UVToViewB;
uniform vec2 u_FocalLen;
uniform vec2 u_LinMAD;

uniform sampler2D s_Tex0;
uniform sampler2D s_Tex1;

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

const float PI = 3.14159265;

uniform float AOStrength = 2.5;
uniform float R = 0.2;
uniform float R2 = 0.2*0.2;
uniform float NegInvR2 = - 1.0 / (0.2*0.2);
uniform float TanBias = tan(30.0 * PI / 180.0);
uniform float MaxRadiusPixels = 100.0;

uniform int NumDirections = 6;
uniform int NumSamples = 4;

float ViewSpaceZFromDepth(float d)
{
	d = d * 2.0 - 1.0;
	return -1.0 / (u_LinMAD.x * d + u_LinMAD.y);
}

vec3 UVToViewSpace(vec2 uv, float z)
{
	uv = u_UVToViewA * uv + u_UVToViewB;
	return vec3(uv * z, z);
}

vec3 GetViewPos(vec2 uv)
{
	float z = ViewSpaceZFromDepth(texture(s_Tex0, uv).r);
	return UVToViewSpace(uv, z);
}

vec3 GetViewPosPoint(ivec2 uv)
{
	ivec2 coord = ivec2(gl_FragCoord.xy) + uv;
	float z = texelFetch(s_Tex0, coord, 0).r;
	return UVToViewSpace(uv, z);
}

float TanToSin(float x)
{
	return x * inversesqrt(x*x + 1.0);
}

float InvLength(vec2 V)
{
	return inversesqrt(dot(V,V));
}

float Tangent(vec3 V)
{
	return V.z * InvLength(V.xy);
}

float BiasedTangent(vec3 V)
{
	return V.z * InvLength(V.xy) + TanBias;
}

float Tangent(vec3 P, vec3 S)
{
    return -(P.z - S.z) * InvLength(S.xy - P.xy);
}

float Length2(vec3 V)
{
	return dot(V,V);
}

vec3 MinDiff(vec3 P, vec3 Pr, vec3 Pl)
{
    vec3 V1 = Pr - P;
    vec3 V2 = P - Pl;
    return (Length2(V1) < Length2(V2)) ? V1 : V2;
}

vec2 SnapUVOffset(vec2 uv)
{
    return round(uv * AORes) * InvAORes;
}

float Falloff(float d2)
{
	return d2 * NegInvR2 + 1.0f;
}

float HorizonOcclusion(	vec2 deltaUV,
						vec3 P,
						vec3 dPdu,
						vec3 dPdv,
						float randstep,
						float numSamples)
{
	float ao = 0;

	// Offset the first coord with some noise
	vec2 uv = fs_TexCoord + SnapUVOffset(randstep*deltaUV);
	deltaUV = SnapUVOffset( deltaUV );

	// Calculate the tangent vector
	vec3 T = deltaUV.x * dPdu + deltaUV.y * dPdv;

	// Get the angle of the tangent vector from the viewspace axis
	float tanH = BiasedTangent(T);
	float sinH = TanToSin(tanH);

	float tanS;
	float d2;
	vec3 S;

	// Sample to find the maximum angle
	for(float s = 1; s <= numSamples; ++s)
	{
		uv += deltaUV;
		S = GetViewPos(uv);
		tanS = Tangent(P, S);
		d2 = Length2(S - P);

		// Is the sample within the radius and the angle greater?
		if(d2 < R2 && tanS > tanH)
		{
			float sinS = TanToSin(tanS);
			// Apply falloff based on the distance
			ao += Falloff(d2) * (sinS - sinH);

			tanH = tanS;
			sinH = sinS;
		}
	}
	
	return ao;
}

vec2 RotateDirections(vec2 Dir, vec2 CosSin)
{
    return vec2(Dir.x*CosSin.x - Dir.y*CosSin.y,
                  Dir.x*CosSin.y + Dir.y*CosSin.x);
}

void ComputeSteps(inout vec2 stepSizeUv, inout float numSteps, float rayRadiusPix, float rand)
{
    // Avoid oversampling if numSteps is greater than the kernel radius in pixels
    numSteps = min(NumSamples, rayRadiusPix);

    // Divide by Ns+1 so that the farthest samples are not fully attenuated
    float stepSizePix = rayRadiusPix / (numSteps + 1);

    // Clamp numSteps if it is greater than the max kernel footprint
    float maxNumSteps = MaxRadiusPixels / stepSizePix;
    if (maxNumSteps < numSteps)
    {
        // Use dithering to avoid AO discontinuities
        numSteps = floor(maxNumSteps + rand);
        numSteps = max(numSteps, 1);
        stepSizePix = MaxRadiusPixels / numSteps;
    }

    // Step size in uv space
    stepSizeUv = stepSizePix * InvAORes;
}

void main()
{
	float numDirections = NumDirections;

	vec3 P, Pr, Pl, Pt, Pb;
	P 	= GetViewPos(fs_TexCoord);

	// Sample neighboring pixels
    Pr 	= GetViewPos(fs_TexCoord + vec2( InvAORes.x, 0));
    Pl 	= GetViewPos(fs_TexCoord + vec2(-InvAORes.x, 0));
    Pt 	= GetViewPos(fs_TexCoord + vec2( 0, InvAORes.y));
    Pb 	= GetViewPos(fs_TexCoord + vec2( 0,-InvAORes.y));

    // Calculate tangent basis vectors using the minimu difference
    vec3 dPdu = MinDiff(P, Pr, Pl);
    vec3 dPdv = MinDiff(P, Pt, Pb) * (AORes.y * InvAORes.x);

    // Get the random samples from the noise texture
	vec3 random = texture(s_Tex1, fs_TexCoord.xy * NoiseScale).rgb;

	// Calculate the projected size of the hemisphere
    vec2 rayRadiusUV = 0.5 * R * u_FocalLen / -P.z;
    float rayRadiusPix = rayRadiusUV.x * AORes.x;

    float ao = 0.0;

    // Make sure the radius of the evaluated hemisphere is more than a pixel
    if(rayRadiusPix > 1.0)
    {
    	ao = 0.0;
    	float numSteps;
    	vec2 stepSizeUV;

    	// Compute the number of steps
    	ComputeSteps(stepSizeUV, numSteps, rayRadiusPix, random.z);

		float alpha = 2.0 * PI / numDirections;

		// Calculate the horizon occlusion of each direction
		for(float d = 0; d < numDirections; ++d)
		{
			float theta = alpha * d;

			// Apply noise to the direction
			vec2 dir = RotateDirections(vec2(cos(theta), sin(theta)), random.xy);
			vec2 deltaUV = dir * stepSizeUV;

			// Sample the pixels along the direction
			ao += HorizonOcclusion(	deltaUV,
									P,
									dPdu,
									dPdv,
									random.z,
									numSteps);
		}

		// Average the results and produce the final AO
		ao = ao / numDirections * AOStrength;
	}

	frag = vec2(ao, clamp(P.z * 30.0, 0.0, 0.99998));
}