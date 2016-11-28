#version 400 core

uniform mat4 u_InvProjMatrix;
uniform mat3 u_InvRotation;

uniform sampler2D _Coverage;
uniform sampler2D _Curl2D;
uniform sampler3D _Perlin3D;
uniform sampler3D _Detail3D;

uniform float _BaseScale;
uniform vec3 _BaseOffset;

uniform float _DetailScale;
uniform vec3 _DetailOffset;

uniform float _CoverageScale;
uniform vec2 _CoverageOffset;
uniform float _MaxRayDistance;

uniform float _HorizonCoverageStart;
uniform float _HorizonCoverageEnd;

uniform vec4 _CloudHeightGradient1;
uniform vec4 _CloudHeightGradient2;
uniform vec4 _CloudHeightGradient3;

uniform vec3 _Random0;
uniform vec3 _Random1;
uniform vec3 _Random2;
uniform vec3 _Random3;
uniform vec3 _Random4;
uniform vec3 _Random5;

uniform float _EarthRadius;
uniform float _StartHeight;
uniform float _EndHeight;
uniform float _AtmosphereThickness;
uniform vec3 _CameraPosition;
uniform float _MaxDistance;
uniform float _RayMinimumY;
uniform float _RayStepLength;
uniform float _MaxIterations;
uniform float _LODDistance;

uniform float _Density;
uniform float _DarkOutlineScalar;
uniform float _ConeRadius;
uniform float _SunRayLength;
uniform float _ForwardScatteringG;
uniform float _BackwardScatteringG;

uniform vec3 _LightDirection;
uniform vec3 _LightColor;

uniform float _CloudDistortionScale;
uniform float _SampleScalar;
uniform float _CloudBottomFade;
uniform float _SampleThreshold;
uniform float _ErosionEdgeSize;
uniform float _CloudDistortion;

uniform float _HorizonFadeScalar;
uniform float _HorizonFadeStartAlpha;
uniform float _OneMinusHorizonFadeStartAlpha;

uniform vec3 _CloudBaseColor;
uniform vec3 _CloudTopColor;

#define VEC4_COVERAGE( f) f.r
#define VEC4_RAIN( f) f.g
#define VEC4_TYPE( f) f.b

-- vs
layout(location = 0) in vec2 vs_Position;
layout(location = 1) in vec2 vs_TexCoord;
out vec3 fs_CameraRay;
out vec2 fs_TexCoord;

vec3 UVToCameraRay(vec2 uv)
{
	vec4 cameraRay;
	cameraRay.x = uv.x * 2.0 - 1.0;
	cameraRay.y = uv.y * 2.0 - 1.0;
	cameraRay.z = 1.0;
	cameraRay.w = 1.0;
	
	cameraRay = u_InvProjMatrix * cameraRay;
	cameraRay = cameraRay / cameraRay.w;
	
	return u_InvRotation * cameraRay.xyz;
}

void main()
{
	gl_Position = vec4(vs_Position, 0, 1);
	fs_CameraRay = UVToCameraRay(vs_TexCoord);
	fs_TexCoord = vs_TexCoord;
}

-- fs
layout(location = 0) out vec4 frag;
in vec3 fs_CameraRay;
in vec2 fs_TexCoord;

float GradientStep(float a, vec4 gradient)
{
	return smoothstep( gradient.x, gradient.y, a) - smoothstep( gradient.z, gradient.w, a);
}

float SmoothThreshold( float value, float threshold, float edgeSize)
{
	return smoothstep( threshold, threshold + edgeSize, value);
}

vec3 SmoothThreshold( vec3 value, float threshold, float edgeSize)
{
	value.r = smoothstep( threshold, threshold + edgeSize, value.r);
	value.g = smoothstep( threshold, threshold + edgeSize, value.g);
	value.b = smoothstep( threshold, threshold + edgeSize, value.b);
	
	return value;
}

float Lerp3(float v0, float v1, float v2, float a)
{
	return a < 0.5 ? mix( v0, v1, a * 2.0) : mix( v1, v2, (a-0.5) * 2.0);
}

vec4 Lerp3(vec4 v0, vec4 v1, vec4 v2, float a)
{
	return vec4(Lerp3( v0.x, v1.x, v2.x, a),
				Lerp3( v0.y, v1.y, v2.y, a),
				Lerp3( v0.z, v1.z, v2.z, a),
				Lerp3( v0.w, v1.w, v2.w, a));
}

float NormalizedAtmosphereY(vec3 ray)
{
	float y = length(ray) - _EarthRadius - _StartHeight;
	return y / _AtmosphereThickness;
}

vec3 InternalRaySphereIntersect(float sphereRadius, vec3 origin, vec3 direction)
{	
	float a0 = sphereRadius * sphereRadius - dot( origin, origin);
	float a1 = dot( origin, direction);
	float result = sqrt(a1 * a1 + a0) - a1;
	
	return origin + direction * result;
}

vec4 SampleCoverage(vec3 ray, float csRayHeight, float lod)
{
	vec2 unit = ray.xz * _CoverageScale;
	vec2 uv = unit * 0.5 + 0.5;
	uv += _CoverageOffset;

	float depth = distance(ray, _CameraPosition) / _MaxDistance;
	vec4 coverage = texture(_Coverage, uv);
	vec4 coverageB = vec4(1.0, 0.0, 0.0, 0.0);
	float alpha = smoothstep(_HorizonCoverageStart, _HorizonCoverageEnd, depth);

	return mix(coverage, vec4(0), alpha);
}

float SampleCloud(vec3 ray, float rayDensity, vec4 coverage, float csRayHeight, float lod)
{
	vec4 coord = vec4(ray * _BaseScale + _BaseOffset, 0.0);
	vec4 noiseSample = texture(_Perlin3D, coord.xyz);
	
	vec4 gradientScalar = vec4(1.0,
		GradientStep(csRayHeight, _CloudHeightGradient1),
		GradientStep(csRayHeight, _CloudHeightGradient2),
		GradientStep(csRayHeight, _CloudHeightGradient3));

	noiseSample *= gradientScalar;

	float noise = clamp((noiseSample.r + noiseSample.g + noiseSample.b + noiseSample.a) / 4.0, 0.0, 1.0);
	
	vec4 gradient = Lerp3(_CloudHeightGradient3,
							_CloudHeightGradient2,
							_CloudHeightGradient1,
							VEC4_COVERAGE(coverage));
							
	noise *= GradientStep( csRayHeight, gradient);

	noise = SmoothThreshold(noise, _SampleThreshold, _ErosionEdgeSize);
	noise = clamp(noise - (1.0 - VEC4_COVERAGE(coverage)), 0.0, 1.0) * VEC4_COVERAGE(coverage);
	
	if (noise > 0.0 && noise < 1.0 && lod == 0)
	{
		vec4 distUV = vec4(ray.xy * _BaseScale * _CloudDistortionScale, 0.0, 0.0);
		vec3 curl = texture(_Curl2D, distUV.xy).xyz * 2.0 - 1.0;

		coord = vec4(ray * _BaseScale * _DetailScale, 0.0);
		coord.xyz += _DetailOffset;

		curl *= _CloudDistortion * csRayHeight;
		coord.xyz += curl;

		vec3 detail = 1.0 - texture(_Detail3D, coord.xyz).xyz;
		detail *= gradientScalar.gba;
		float detailValue = detail.r + detail.g + detail.b;
		detailValue /= 3.0;
		detailValue *= smoothstep( 1.0, 0.0, noise) * 0.5;
		noise -= detailValue;

		noise = clamp(noise, 0.0, 1.0);
	}

	return noise * _SampleScalar * smoothstep(0.0, _CloudBottomFade * 1.0, csRayHeight);
}

float HenyeyGreensteinPhase(float cosAngle, float g)
{
	float g2 = g * g;
	return (1.0 - g2) / pow( 1.0 + g2 - 2.0 * g * cosAngle, 1.5);
}

float BeerTerm(float densityAtSample)
{
	return exp(-_Density * densityAtSample);
}
			
float PowderTerm(float densityAtSample, float cosTheta)
{
	float powder = 1.0 - exp(-_Density * densityAtSample * 2.0);
	powder = clamp(powder * _DarkOutlineScalar * 2.0, 0.0, 1.0);
	return mix( 1.0, powder, smoothstep( 0.5, -0.5, cosTheta));
}

vec3 SampleLight(vec3 origin, float originDensity, float pixelAlpha, float cosAngle, vec2 debugUV, float rayDistance, vec3 RandomUnitSphere[6])
{
	float iterations = 5.0;
	
	vec3 rayStep = _LightDirection * (_SunRayLength / iterations);
	vec3 ray = origin + rayStep;
	
	float atmosphereY = 0.0;

	float lod = step( 0.3, originDensity) * 3.0;
	lod = 0.0;

	float value = 0.0;

	vec4 coverage;

	vec3 randomOffset = vec3( 0.0, 0.0, 0.0);
	float coneRadius = 0.0;
	float coneStep = _ConeRadius / iterations;
	float energy = 0.0;

	float thickness = 0.0;

	for( float i=0.0; i < iterations; i++)
	{
		randomOffset = RandomUnitSphere[int(i)] * coneRadius;
		ray += rayStep;
		atmosphereY = NormalizedAtmosphereY(ray);

		coverage = SampleCoverage(ray + randomOffset, atmosphereY, lod);
		value = SampleCloud(ray + randomOffset, originDensity, coverage, atmosphereY, lod);
		value *= float( atmosphereY <= 1.0);

		thickness += value;

		coneRadius += coneStep;
	}

	float far = 8.0;
	ray += rayStep * far;
	atmosphereY = NormalizedAtmosphereY( ray);
	coverage = SampleCoverage(ray, atmosphereY, lod);
	value = SampleCloud(ray, originDensity, coverage, atmosphereY, lod);
	value *= float(atmosphereY <= 1.0);
	thickness += value;

	float forwardP = HenyeyGreensteinPhase(cosAngle, _ForwardScatteringG);
	float backwardsP = HenyeyGreensteinPhase(cosAngle, _BackwardScatteringG);
	float P = (forwardP + backwardsP) / 2.0;

	return _LightColor * BeerTerm( thickness) * PowderTerm(originDensity, cosAngle) * P;
}

vec3 SampleAmbientLight(float atmosphereY, float depth)
{
	return mix(_CloudBaseColor, _CloudTopColor, atmosphereY);
}

void main()
{
	vec3 rayDirection = normalize(fs_CameraRay);
	vec4 color = vec4(0.0, 0.0, 0.0, 0.0);
	
	if(rayDirection.y > _RayMinimumY)
	{
		vec2 uv = fs_TexCoord;
		vec3 ray = InternalRaySphereIntersect(_EarthRadius + _StartHeight, _CameraPosition, rayDirection);
		vec3 rayStep = rayDirection * _RayStepLength;
		float i = 0;
		
		float atmosphereY = 0.0;
		float transmittance = 1.0;
		float rayStepScalar = 1.0;
		float cosAngle = dot(rayDirection, _LightDirection);
		
		float normalizedDepth = 0.0;
		float zeroThreshold = 4.0;
		float zeroAccumulator = 0.0;
		vec3 RandomUnitSphere[6] = vec3[](_Random0, _Random1, _Random2, _Random3, _Random4, _Random5);
		float value = 1.0;
		
		while(true)
		{
			if(i >= _MaxIterations || color.a >= 1.0 || atmosphereY >= 1.0) {
				break;
			}
			
			normalizedDepth = distance(_CameraPosition, ray) / _MaxDistance;
			float lod = step(_LODDistance, normalizedDepth);
            
			vec4 coverage = SampleCoverage(ray, atmosphereY, lod);
			if (coverage == vec4(0)) {
				break;
			}
			
			value = SampleCloud(ray, color.a, coverage, atmosphereY, lod);
			vec4 particle = vec4(value, value, value, value);
			
			if(value > 0.0)
			{
				zeroAccumulator = 0.0;
				
				if( rayStepScalar > 1.0)
				{
					ray -= rayStep * rayStepScalar;
					i -= rayStepScalar;

					atmosphereY = NormalizedAtmosphereY( ray);
					normalizedDepth = distance(_CameraPosition, ray) / _MaxDistance;
					lod = step(_LODDistance, normalizedDepth);
					coverage = SampleCoverage( ray, atmosphereY, lod);
					value = SampleCloud(ray, color.a, coverage, atmosphereY, lod);
					particle = vec4( value, value, value, value);
				}
				
				float T = 1.0 - particle.a;
				transmittance *= T;
				
				vec3 ambientLight = SampleAmbientLight(atmosphereY, normalizedDepth);
				vec3 sunLight = SampleLight(ray, particle.a, color.a, cosAngle, uv, normalizedDepth, RandomUnitSphere);
				
				particle.a = 1.0 - T;
				particle.rgb = sunLight + ambientLight;
				particle.rgb *= particle.a;
				
				color = (1.0 - color.a) * particle + color;
			}
			
			zeroAccumulator += float(value <= 0.0);
			rayStepScalar = 1.0 + step(zeroThreshold, zeroAccumulator) * 0.0;
			i += rayStepScalar;

			ray += rayStep * rayStepScalar;
			atmosphereY = NormalizedAtmosphereY(ray);
		}
		
		float fade = smoothstep(_RayMinimumY, _RayMinimumY + (1.0 - _RayMinimumY) * _HorizonFadeScalar, rayDirection.y);							
		color *= _HorizonFadeStartAlpha + fade * _OneMinusHorizonFadeStartAlpha;
	}
	
	frag = color;
}