#version 400 core

uniform mat4 u_ViewProjMatrix;
uniform mat4 u_ModelMatrix;

uniform mat4 u_InvViewProjMatrix;
uniform vec3 u_WorldCamPosition;

uniform sampler2D s_Tex0; // depth
uniform sampler2D s_Tex1; // albedo
uniform sampler2D s_Tex2; // normal
uniform sampler2D s_Tex3; // occlusion

uniform vec3 u_LocalCubemapPosition;
uniform vec3 u_LocalCubemapExtent;
uniform vec3 u_LocalCubemapOffset;
uniform bool u_LocalPass;

uniform samplerCube s_GlobalCubemap;
uniform sampler2D s_BRDF;
uniform samplerCube s_LocalCubemap;

-- vs
layout(location = 0) in vec3 vs_Position;
out vec3 fs_ProjCoord;

void main()
{	
	gl_Position = u_ViewProjMatrix * u_ModelMatrix * vec4(vs_Position.xyz, 1);
	fs_ProjCoord.x = (gl_Position.x + gl_Position.w) * 0.5;
	fs_ProjCoord.y = (gl_Position.y + gl_Position.w) * 0.5;
	fs_ProjCoord.z = gl_Position.w;
}

-- fs
layout(location = 0) out vec4 frag;
in vec3 fs_ProjCoord;

vec3 GetWorldPos(vec2 uv, float depth)
{
	vec4 pos = vec4(uv, depth, 0.0) * 2.0 - 1.0;
	pos.w = 1.0;

	pos = u_InvViewProjMatrix * pos;
	pos /= pos.w;

	return pos.xyz;
}

vec3 getBoxIntersection( vec3 pos, vec3 reflectionVector, vec3 cubeSize, vec3 cubePos )
{
	vec3 rbmax = (   0.5f * ( cubeSize - cubePos ) - pos ) / reflectionVector;
	vec3 rbmin = ( - 0.5f * ( cubeSize - cubePos ) - pos ) / reflectionVector;
	
	vec3 rbminmax = vec3(
		( reflectionVector.x > 0.0f ) ? rbmax.x : rbmin.x,
		( reflectionVector.y > 0.0f ) ? rbmax.y : rbmin.y,
		( reflectionVector.z > 0.0f ) ? rbmax.z : rbmin.z );
	
	float correction = min( min( rbminmax.x, rbminmax.y ), rbminmax.z );
	return ( pos + reflectionVector * correction );
}

float computeDistanceBaseRoughness( float distInteresectionToShadedPoint, float distInteresectionToProbeCenter, float linearRoughness )
{
	float newLinearRoughness = clamp ( distInteresectionToShadedPoint /
									  distInteresectionToProbeCenter * linearRoughness , 0.0, linearRoughness );
	return mix( newLinearRoughness , linearRoughness , linearRoughness );
}

void main()
{
    float depth = textureProj(s_Tex0, fs_ProjCoord).r;
    vec4 albedo = textureProj(s_Tex1, fs_ProjCoord);
    vec4 normal = textureProj(s_Tex2, fs_ProjCoord);
	float ao = 1.0 - textureProj(s_Tex3, fs_ProjCoord).r;
	
	float roughness = clamp(albedo.a, 0.0, 1.0);
    float metalness = clamp(normal.a, 0.0, 1.0);
	
    vec2 uv = fs_ProjCoord.xy / fs_ProjCoord.z; 
    vec3 wpos = GetWorldPos(uv, depth) - u_LocalCubemapOffset;

	vec3 N = normalize(normal.xyz * 2.0 - 1.0);
	vec3 V = normalize((u_WorldCamPosition - u_LocalCubemapOffset) - wpos);
	vec3 R = reflect(-V, N);
	
	float ndotv = clamp(dot(N, V), 0.0, 1.0);
	
    vec2 f0_scale_bias = texture(s_BRDF, vec2(ndotv, roughness)).rg;
    vec3 F0 = mix(vec3(0.04), vec3(1.0), metalness);
	vec3 F = F0 * f0_scale_bias.x + vec3(f0_scale_bias.y);
	
	float distRough = computeDistanceBaseRoughness(length(wpos), 0.0f, 0.09f + roughness);
	float numMips	= 7.0;
	float mip = 1.0 - 1.2 * log2( distRough );
	mip = numMips - 1.0 - mip;
	
	vec4 color = textureLod(s_GlobalCubemap, R, mip);
	
	if (wpos.x > u_LocalCubemapPosition.x - u_LocalCubemapExtent.x && wpos.x < u_LocalCubemapPosition.x + u_LocalCubemapExtent.x &&
		wpos.y > u_LocalCubemapPosition.y - u_LocalCubemapExtent.y && wpos.y < u_LocalCubemapPosition.y + u_LocalCubemapExtent.y &&
		wpos.z > u_LocalCubemapPosition.z - u_LocalCubemapExtent.z && wpos.z < u_LocalCubemapPosition.x + u_LocalCubemapExtent.z)
	{
		if (!u_LocalPass) {
			discard;
		}
		
		vec3 boxInters = getBoxIntersection(wpos, R, vec3(93, 52, 45.5), u_LocalCubemapPosition);
		vec3 lookup = boxInters - u_LocalCubemapPosition;
		
		distRough = computeDistanceBaseRoughness(length(wpos - boxInters), length(u_LocalCubemapPosition - boxInters), 0.09f + roughness);
		mip = 1.0 - 1.2 * log2(distRough);
		mip = numMips - 1.0 - mip;
		
		vec4 local_color = textureLod(s_LocalCubemap, lookup, mip);		
		color = mix(color, local_color, local_color.a);
	}
	
	frag = vec4(color.rgb * F * ao, 1);
}