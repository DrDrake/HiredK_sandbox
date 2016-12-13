#version 400 core

uniform mat4 u_ViewProjMatrix;
uniform mat4 u_ModelMatrix;
uniform mat3 u_NormalMatrix;

uniform sampler2D s_Albedo;
uniform sampler2D s_Normal;

uniform vec3 u_WorldCamPosition;
uniform vec3 u_SunDirection;

-- vs
layout(location = 0) in vec3 vs_Position;
layout(location = 1) in vec2 vs_TexCoord;
layout(location = 2) in vec4 vs_Color;
layout(location = 3) in vec3 vs_Normal;
layout(location = 4) in vec3 vs_Tangent;
out vec3 fs_Position;
out vec2 fs_TexCoord;
out vec3 fs_TangentViewPos;
out vec3 fs_TangentFragPos;
out vec3 fs_TangentLightPos;
out mat3 fs_TBN;

void main()
{	
	fs_Position = (u_ModelMatrix * vec4(vs_Position.xyz, 1)).xyz;
	gl_Position = u_ViewProjMatrix * vec4(fs_Position.xyz, 1);  
    fs_TexCoord = vs_TexCoord;
	
	vec3 n = u_NormalMatrix * vs_Normal;
    vec3 t = u_NormalMatrix * vs_Tangent;
	vec3 L = normalize(u_SunDirection);
	
	vec3 N = normalize(n);
	vec3 T = normalize(t - dot(t, N) * N);
	vec3 B = cross(N, T);
	
	fs_TBN = mat3(T, B, N);
	
	// temp
	fs_TangentViewPos = transpose(fs_TBN) * u_WorldCamPosition;
	fs_TangentFragPos = transpose(fs_TBN) * fs_Position;
	fs_TangentLightPos = transpose(fs_TBN) * L;
}

-- fs
layout(location = 0) out vec4 albedo;
layout(location = 1) out vec4 normal;
in vec3 fs_Position;
in vec2 fs_TexCoord;
in vec3 fs_TangentViewPos;
in vec3 fs_TangentFragPos;
in vec3 fs_TangentLightPos;
in mat3 fs_TBN;

float height_scale = 0.08f;

vec2 ParallaxMapping(in vec3 V, in vec2 T, out float parallaxHeight)
{
   const float minLayers = 10;
   const float maxLayers = 15;
   float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0, 0, 1), V)));

   float layerHeight = 1.0 / numLayers;
   float curLayerHeight = 0;
   vec2 dtex = height_scale * V.xy / V.z / numLayers;

   vec2 currentTextureCoords = T;

   float heightFromTexture = texture(s_Normal, currentTextureCoords).a;
   
   while(heightFromTexture > curLayerHeight) 
   {
      curLayerHeight += layerHeight; 
      currentTextureCoords -= dtex;
      heightFromTexture = texture(s_Normal, currentTextureCoords).a;
   }
   
   vec2 prevTCoords = currentTextureCoords + dtex;
   float nextH	= heightFromTexture - curLayerHeight;
   float prevH	= texture(s_Normal, prevTCoords).a - curLayerHeight + layerHeight;
   float weight = nextH / (nextH - prevH);

   vec2 finalTexCoords = prevTCoords * weight + currentTextureCoords * (1.0-weight);
   parallaxHeight = curLayerHeight + prevH * weight + nextH * (1.0 - weight);
   return finalTexCoords;
}

float ParallaxSoftShadowMultiplier(in vec3 L, in vec2 initialTexCoord, in float initialHeight)
{
	float shadowMultiplier = 1;
	const float minLayers = 15;
	const float maxLayers = 30;
	
	if (dot(vec3(0, 0, 1), L) > 0)
	{
		float numSamplesUnderSurface = 0;
		
		shadowMultiplier = 0;
		float numLayers	= mix(maxLayers, minLayers, abs(dot(vec3(0, 0, 1), L)));
		float layerHeight = initialHeight / numLayers;
		vec2 texStep = height_scale * L.xy / L.z / numLayers;
		
		float currentLayerHeight = initialHeight - layerHeight;
		vec2 currentTextureCoords = initialTexCoord + texStep;
		float heightFromTexture	= texture(s_Normal, currentTextureCoords).a;
		int stepIndex	= 1;
		
		while (currentLayerHeight > 0 && stepIndex < maxLayers)
		{
			if (heightFromTexture < currentLayerHeight)
			{
				numSamplesUnderSurface += 1;
				float newShadowMultiplier = (currentLayerHeight - heightFromTexture) * (1.0 - stepIndex / numLayers);
				shadowMultiplier = max(shadowMultiplier, newShadowMultiplier);
			}
			
			stepIndex += 1;
			currentLayerHeight -= layerHeight;
			currentTextureCoords += texStep;
			heightFromTexture = texture(s_Normal, currentTextureCoords).a;
		}
		
		shadowMultiplier = (numSamplesUnderSurface < 1) ? 1.0 : 1.0 - shadowMultiplier;
	}

	return shadowMultiplier;
}

void main()
{
	vec3 V = normalize(fs_TangentViewPos - fs_TangentFragPos);
	vec3 L = normalize(fs_TangentLightPos);
	
	float parallaxHeight;
	vec2 T = ParallaxMapping(V, fs_TexCoord, parallaxHeight);
	if (T.x > 1.0 || T.y > 1.0 || T.x < 0.0 || T.y < 0.0) {
		discard;
	}
	
	float shadow = ParallaxSoftShadowMultiplier(L, T, parallaxHeight - 0.05);
	vec4 color = texture(s_Albedo, T);
	color.rgb *= shadow;
	
	vec3 n = normalize(texture(s_Normal, T).rgb * 2.0 - 1.0);
	
	n = n * vec3(0.2f, 0.2f, 1.0); // temp smooth normal
	
	n = normalize(fs_TBN * n);
	
	albedo = vec4(color.rgb, color.a);
    normal = vec4(n * 0.5 + 0.5, 0);
}