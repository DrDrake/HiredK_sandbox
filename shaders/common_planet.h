
uniform mat4 u_Deform_ScreenQuadCorners;
uniform mat4 u_Deform_ScreenQuadVerticals;
uniform vec4 u_Deform_ScreenQuadCornerNorms;
uniform vec4 u_Deform_Offset;
uniform float u_Deform_Radius;
uniform mat4 u_Deform_TangentFrameToWorld;
uniform mat3 u_Deform_LocalToWorld;
uniform vec2 u_Deform_Blending;
uniform vec4 u_Deform_Camera;

uniform vec3 u_HeightNormal_TileSize;
uniform vec3 u_HeightNormal_TileCoords;
uniform sampler2D s_HeightNormal_Slot0;
uniform sampler2D s_HeightNormal_Slot1;
uniform sampler2D s_HeightNormal_Slot2;

const float SAND_TEXTURE_SCALE  = 100.0f;
const float GRASS_TEXTURE_SCALE = 100.0f;
const float ROCK_TEXTURE_SCALE  = 100.0f;
const float ROCK_PARALLAX_SCALE = 0.008f;

uniform sampler2DArray s_AlbedoDetailTexArray;
uniform sampler2DArray s_NormalDetailTexArray;

uniform vec3 u_WorldCamPosition;
uniform vec2 u_CameraClip;
uniform bool u_Spherical;
uniform vec3 u_SunDirection;

uniform float u_DeferredDist;
uniform float u_DetailsDist;
uniform float u_ParallaxDist;
uniform bool u_DrawQuadtree;

uniform vec4 u_Pencil;
uniform vec4 u_PencilColor;
uniform bool u_DrawPencil;

vec4 texTile(sampler2D tile, vec2 uv, vec3 tileCoords, vec3 tileSize)
{
	uv = tileCoords.xy + uv * tileSize.xy;
	return texture2D(tile, uv);
}

vec4 getSphericalPos(vec2 pos, vec2 uv, out vec3 deformed)
{
    vec2 zfc = texTile(s_HeightNormal_Slot0, uv, u_HeightNormal_TileCoords, u_HeightNormal_TileSize).xy;
	if	(!u_Spherical && zfc.x < 0.0) {
		zfc = vec2(0, 0);
	}
    
	vec2 vert = abs(u_Deform_Camera.xy - pos);
	float d = max(max(vert.x, vert.y), u_Deform_Camera.z);
	float blend = clamp((d - u_Deform_Blending.x) / u_Deform_Blending.y, 0.0, 1.0);
    
	float R = u_Deform_Radius;
	mat4 C = u_Deform_ScreenQuadCorners;
	mat4 N = u_Deform_ScreenQuadVerticals;
	vec4 L = u_Deform_ScreenQuadCornerNorms;
	vec3 P = vec3(pos * u_Deform_Offset.z + u_Deform_Offset.xy, R);
    
	vec4 uvUV = vec4(pos, vec2(1.0, 1.0) - pos);
	vec4 alpha = uvUV.zxzx * uvUV.wwyy;
	vec4 alphaPrime = alpha * L / dot(alpha, L);
    
	float h = zfc.x * (1.0 - blend) + zfc.y * blend;
	float k = min(length(P) / dot(alpha, L) * 1.0000003, 1.0);
	float hPrime = (h + R * (1.0 - k)) / k;	
    
	deformed = (u_Deform_Radius + h) * normalize(u_Deform_LocalToWorld * P);
    return (C + hPrime * N) * alphaPrime;
}

vec4 getPos(vec2 pos, vec2 uv, out vec3 deformed)
{
	vec2 zfc = texTile(s_HeightNormal_Slot0, uv, u_HeightNormal_TileCoords, u_HeightNormal_TileSize).xy;
	if	(!u_Spherical && zfc.x < 0.0) {
		zfc = vec2(0, 0);
	}
	
	vec2 vert = abs(u_Deform_Camera.xy - pos.xy);   
	float d = max(max(vert.x, vert.y), u_Deform_Camera.z);   
	float blend = clamp((d - u_Deform_Blending.x) / u_Deform_Blending.y, 0.0, 1.0);
	
    mat4 C = u_Deform_ScreenQuadCorners;
    mat4 N = u_Deform_ScreenQuadVerticals;
    
    vec4 uvUV = vec4(pos.xy, vec2(1.0) - pos.xy);
    vec4 alpha = uvUV.zxzx * uvUV.wwyy;
	float h = zfc.x * (1.0 - blend) + zfc.y * blend;
	
	vec3 p = vec3(pos.xy * u_Deform_Offset.z + u_Deform_Offset.xy, h);
	deformed = vec3(p.x, h, p.y); // temp, could be done better?
    return (C + h * N) * alpha;
}

float GetDetailsBlendFactor() {
	return (gl_FragCoord.z < u_DetailsDist) ? 1.0 : (1.0 - smoothstep(u_DetailsDist, u_DetailsDist + 0.0004f, gl_FragCoord.z));
}

float GetParallaxBlendFactor() {
	return (gl_FragCoord.z < u_ParallaxDist) ? 1.0 : (1.0 - smoothstep(u_ParallaxDist, u_ParallaxDist + 0.002f, gl_FragCoord.z));
}

vec3 BlendNormals(vec3 N, vec3 dn)
{
	float a = 1 / ( 1 + N.z);
	float b = -N.x * N.y * a;
	
	vec3 b1 = vec3(1 - N.x * N.x * a, b, -N.x);
	vec3 b2 = vec3(b, 1 - N.y * N.y * a, -N.y);
	vec3 b3 = N;
	
	if (N.z < -0.9999999) {
		b1 = vec3(0, -1, 0);
		b2 = vec3(-1, 0, 0);
	}
	
	return normalize(dn.x * b1 + dn.y * b2 + dn.z * b3);
}

vec4 RealisticBlend(vec4 c1, float h1, vec4 c2, float h2)
{
    float depth = 0.02;
    float ma = max(h1, h2) - depth;
    float b1 = max(h1 - ma, 0);
    float b2 = max(h2 - ma, 0);

    return (c1 * b1 + c2 * b2) / (b1 + b2);
}

vec4 ImprovedTiling(sampler2DArray tex, vec2 uv, float index, float strength)
{
    vec4 c1 = texture(tex, vec3(uv, index));
    vec4 c2 = texture(tex, vec3(uv * -0.25, index));
    return mix(c1, c2, strength);
}

vec2 RockParallaxMapping(vec2 uv, vec3 V, out float height)
{
   const float minLayers = 10;
   const float maxLayers = 15;
   float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0, 0, 1), V)));
   
   float layerHeight = 1.0 / numLayers;
   float curLayerHeight = 0;
   vec2 step = ROCK_PARALLAX_SCALE * V.xy / V.z / numLayers;  
   vec2 currentTextureCoords = uv;

   float heightFromTexture = ImprovedTiling(s_NormalDetailTexArray, currentTextureCoords * ROCK_TEXTURE_SCALE, 2, 0.5).a;
   while (heightFromTexture > curLayerHeight) 
   {
		curLayerHeight += layerHeight; 
		currentTextureCoords -= step;
		heightFromTexture = ImprovedTiling(s_NormalDetailTexArray, currentTextureCoords * ROCK_TEXTURE_SCALE, 2, 0.5).a;
   }
   
   vec2 prevTCoords = currentTextureCoords + step;
   float nextH	= heightFromTexture - curLayerHeight;
   float prevH	= ImprovedTiling(s_NormalDetailTexArray, prevTCoords * ROCK_TEXTURE_SCALE, 2, 0.5).a - curLayerHeight + layerHeight;
   float w = nextH / (nextH - prevH);
   
   vec2 finalTexCoords = prevTCoords * w + currentTextureCoords * (1.0 - w);
   height = curLayerHeight + prevH * w + nextH * (1.0 - w);
   return finalTexCoords * ROCK_TEXTURE_SCALE;
}

float RockParallaxSoftShadow(vec2 uv, vec3 L, float height)
{
	float shadowMultiplier = 1;
	const float minLayers = 15;
	const float maxLayers = 30;

	if (dot(vec3(0, 0, 1), L) > 0)
	{
		float numSamplesUnderSurface = 0;
		shadowMultiplier = 0;
		
		float numLayers	= mix(maxLayers, minLayers, abs(dot(vec3(0, 0, 1), L)));
		float layerHeight = height / numLayers;
		vec2 step = ROCK_PARALLAX_SCALE * L.xy / L.z / numLayers;
		float currentLayerHeight = height - layerHeight;
		vec2 currentTextureCoords = uv + step;
		
		float heightFromTexture	= ImprovedTiling(s_NormalDetailTexArray, currentTextureCoords, 2, 0.5).a;
		int stepIndex = 1;

		while (currentLayerHeight > 0 && stepIndex < maxLayers)
		{
			if (heightFromTexture < currentLayerHeight) {
				float newShadowMultiplier = (currentLayerHeight - heightFromTexture) * (1.0 - stepIndex / numLayers);
				shadowMultiplier = max(shadowMultiplier, newShadowMultiplier);
				numSamplesUnderSurface += 1;
			}

			currentLayerHeight -= layerHeight;
			currentTextureCoords += step;
			heightFromTexture = ImprovedTiling(s_NormalDetailTexArray, currentTextureCoords, 2, 0.5).a;			
			stepIndex += 1;
		}

		shadowMultiplier = (numSamplesUnderSurface < 1) ? 1.0 : 1.0 - shadowMultiplier;
	}

	return shadowMultiplier;
}

vec4 ProceduralShading(vec2 uv, out vec3 N)
{
	vec4 splat = texTile(s_HeightNormal_Slot2, uv, u_HeightNormal_TileCoords, u_HeightNormal_TileSize);
    vec3 fn = texTile(s_HeightNormal_Slot1, uv, u_HeightNormal_TileCoords, u_HeightNormal_TileSize).xyz * 2.0 - 1.0;
	fn.z = sqrt(max(0.0, 1.0 - dot(fn.xy, fn.xy)));		
	mat3 TTW = mat3(u_Deform_TangentFrameToWorld);
	
	float details_blend_factor = GetDetailsBlendFactor();
	float parallax_blend_factor = GetParallaxBlendFactor();
	
	vec4 result_albedo = vec4(0, 0, 0, 0);
	result_albedo = textureLod(s_AlbedoDetailTexArray, vec3(uv, 0), 16.0);
	result_albedo = mix(result_albedo, textureLod(s_AlbedoDetailTexArray, vec3(uv, 1), 16.0), splat.g);
	result_albedo = mix(result_albedo, textureLod(s_AlbedoDetailTexArray, vec3(uv, 2), 16.0), splat.b);
	
	vec4 rock_normal = texture(s_NormalDetailTexArray, vec3(uv, 2));
	rock_normal.rgb = mix(fn, normalize(rock_normal.rgb * 2.0 - 1.0), splat.b * (1.0 - details_blend_factor) * 0.5);
	fn = BlendNormals(fn, rock_normal.rgb);
	vec2 rock_uv = uv * ROCK_TEXTURE_SCALE;
	float rock_height = 0.0f;
	float rock_shadow = 1.0f;
	
	if (details_blend_factor > 0.01)
	{
		vec4 ar1 = texture(s_AlbedoDetailTexArray, vec3(uv * SAND_TEXTURE_SCALE, 0));
		vec4 ar2 = texture(s_AlbedoDetailTexArray, vec3(uv * GRASS_TEXTURE_SCALE, 1));
		vec4 ar3 = ImprovedTiling(s_AlbedoDetailTexArray, rock_uv,  2, 0.5);
		ar3.rgb *= rock_shadow;
		
		vec4 nh1 = texture(s_NormalDetailTexArray, vec3(uv * SAND_TEXTURE_SCALE, 0));
		vec4 nh2 = texture(s_NormalDetailTexArray, vec3(uv * GRASS_TEXTURE_SCALE, 1));
		vec4 nh3 = ImprovedTiling(s_NormalDetailTexArray, rock_uv,  3, 0.5);
		
		nh1.rgb = normalize(nh1.rgb * 2.0 - 1.0);
		nh2.rgb = normalize(nh2.rgb * 2.0 - 1.0);
		nh3.rgb = normalize(nh3.rgb * 2.0 - 1.0);
		
		float h1 = nh1.a * splat.r * details_blend_factor;
		float h2 = nh2.a * splat.g * details_blend_factor;
		float h3 = (rock_normal.a + nh3.a * 0.5) * splat.b * details_blend_factor;
		
		result_albedo = mix(result_albedo, ar1, splat.r);
		result_albedo = mix(result_albedo, RealisticBlend(result_albedo, max(h1, h3), ar2, h2), details_blend_factor);
		result_albedo = mix(result_albedo, RealisticBlend(result_albedo, max(h1, h2), ar3, h3), details_blend_factor);
		
		fn = mix(fn, BlendNormals(fn, nh1.rgb), splat.r);
		fn = mix(fn, RealisticBlend(vec4(fn, 0), max(h1, h3), vec4(BlendNormals(fn, nh2.rgb), 0), h2).rgb, details_blend_factor);
		fn = mix(fn, RealisticBlend(vec4(fn, 0), max(h1, h2), vec4(BlendNormals(fn, nh3.rgb), 0), h3).rgb, details_blend_factor);
	}
	
	N = normalize(TTW * vec3(-fn.x, -fn.y, fn.z));
	return result_albedo;
}