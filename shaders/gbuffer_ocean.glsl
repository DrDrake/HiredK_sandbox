#version 400 core

const float CHOPPY_FACTOR = 2.5;

uniform mat4 _Ocean_CameraToScreen;
uniform mat4 _Ocean_ScreenToCamera;
uniform mat4 _Ocean_CameraToOcean;
uniform mat3 _Ocean_OceanToCamera;
uniform mat4 _Ocean_OceanToWorld;
uniform vec3 _Ocean_Horizon1;
uniform vec3 _Ocean_Horizon2;
uniform vec3 _Ocean_CameraPos;
uniform vec2 _Ocean_ScreenGridSize;
uniform float _Ocean_Radius;

uniform sampler2DArray fftWavesSampler;
uniform sampler3D slopeVarianceSampler;
uniform vec4 GRID_SIZES;

-- vs
layout(location = 0) in vec4 vs_Position;
out vec3 fs_Position;
out vec2 fs_TexCoord;

vec2 oceanPos(vec3 vertex, out float t, out vec3 cameraDir, out vec3 oceanDir) {
    float horizon = _Ocean_Horizon1.x + _Ocean_Horizon1.y * vertex.x - sqrt(_Ocean_Horizon2.x + (_Ocean_Horizon2.y + _Ocean_Horizon2.z * vertex.x) * vertex.x);
    cameraDir = normalize((_Ocean_ScreenToCamera * vec4(vertex.x, min(vertex.y, horizon), 0.0, 1.0)).xyz);
    oceanDir = (_Ocean_CameraToOcean * vec4(cameraDir, 0.0)).xyz;
    float cz = _Ocean_CameraPos.z;
    float dz = oceanDir.z;
	
	float b = dz * (cz + _Ocean_Radius);
	float c = cz * (cz + 2.0 * _Ocean_Radius);
	float tSphere = - b - sqrt(max(b * b - c, 0.0));
	float tApprox = - cz / dz * (1.0 + cz / (2.0 * _Ocean_Radius) * (1.0 - dz * dz));
	t = abs((tApprox - tSphere) * dz) < 1.0 ? tApprox : tSphere;
	
    return _Ocean_CameraPos.xy + t * oceanDir.xy;
}

vec2 oceanPos(vec3 vertex) {
    float t;
    vec3 cameraDir;
    vec3 oceanDir;
    return oceanPos(vertex, t, cameraDir, oceanDir);
}

void main()
{
	float t;
    vec3 cameraDir;
    vec3 oceanDir;
	
	vec3 vertex = vs_Position.xyz;
	vertex.xy *= 1.25f;
	
    vec2 u = oceanPos(vertex, t, cameraDir, oceanDir);
    vec2 dux = oceanPos(vertex + vec3(_Ocean_ScreenGridSize.x, 0.0, 0.0)) - u;
    vec2 duy = oceanPos(vertex + vec3(0.0, _Ocean_ScreenGridSize.y, 0.0)) - u;
    vec3 dP = vec3(0.0, 0.0, 0.0);
	
    if (duy.x != 0.0 || duy.y != 0.0) {
        dP.z += textureGrad(fftWavesSampler, vec3(u / GRID_SIZES.x, 0.0), dux / GRID_SIZES.x, duy / GRID_SIZES.x).x;
        dP.z += textureGrad(fftWavesSampler, vec3(u / GRID_SIZES.y, 0.0), dux / GRID_SIZES.y, duy / GRID_SIZES.y).y;
        dP.z += textureGrad(fftWavesSampler, vec3(u / GRID_SIZES.z, 0.0), dux / GRID_SIZES.z, duy / GRID_SIZES.z).z;
        dP.z += textureGrad(fftWavesSampler, vec3(u / GRID_SIZES.w, 0.0), dux / GRID_SIZES.w, duy / GRID_SIZES.w).w;
		
        dP.xy += CHOPPY_FACTOR * textureGrad(fftWavesSampler, vec3(u / GRID_SIZES.x, 3.0), dux / GRID_SIZES.x, duy / GRID_SIZES.x).xy;
        dP.xy += CHOPPY_FACTOR * textureGrad(fftWavesSampler, vec3(u / GRID_SIZES.y, 3.0), dux / GRID_SIZES.y, duy / GRID_SIZES.y).zw;
        dP.xy += CHOPPY_FACTOR * textureGrad(fftWavesSampler, vec3(u / GRID_SIZES.z, 4.0), dux / GRID_SIZES.z, duy / GRID_SIZES.z).xy;
        dP.xy += CHOPPY_FACTOR * textureGrad(fftWavesSampler, vec3(u / GRID_SIZES.w, 4.0), dux / GRID_SIZES.w, duy / GRID_SIZES.w).zw;
    }
	
    gl_Position = _Ocean_CameraToScreen * vec4(t * cameraDir + _Ocean_OceanToCamera * dP, 1.0);   
	fs_Position = vec3(0.0, 0.0, _Ocean_CameraPos.z) + t * oceanDir + dP;
	fs_TexCoord = u;
}

-- fs
layout(location = 0) out vec4 albedo;
layout(location = 1) out vec4 normal;
in vec3 fs_Position;
in vec2 fs_TexCoord;

void main()
{
    vec2 slopes = texture(fftWavesSampler, vec3(fs_TexCoord / GRID_SIZES.x, 1.0)).xy;
    slopes += texture(fftWavesSampler, vec3(fs_TexCoord / GRID_SIZES.y, 1.0)).zw;
    slopes += texture(fftWavesSampler, vec3(fs_TexCoord / GRID_SIZES.z, 2.0)).xy;
    slopes += texture(fftWavesSampler, vec3(fs_TexCoord / GRID_SIZES.w, 2.0)).zw;
	slopes -= fs_Position.xy / (_Ocean_Radius + fs_Position.z);
	
	vec3 V = normalize(vec3(0, 0, _Ocean_CameraPos.z) - fs_Position);
    vec3 N = normalize(vec3(-slopes.x, -slopes.y, 1.0));
    if (dot(V, N) < 0.0) {
        N = reflect(N, V);
    }
	
	float Jxx = dFdx(fs_TexCoord.x);
    float Jxy = dFdy(fs_TexCoord.x);
    float Jyx = dFdx(fs_TexCoord.y);
    float Jyy = dFdy(fs_TexCoord.y);
    float A = Jxx * Jxx + Jyx * Jyx;
    float B = Jxx * Jxy + Jyx * Jyy;
    float C = Jxy * Jxy + Jyy * Jyy;
    const float SCALE = 10.0;
    float ua = pow(A / SCALE, 0.25);
    float ub = 0.5 + 0.5 * B / sqrt(A * C);
    float uc = pow(C / SCALE, 0.25);
    
    float roughness = max(texture(slopeVarianceSampler, vec3(ua, ub, uc)).x, 2e-5);
	vec3 fn = normalize(mat3(_Ocean_OceanToWorld) * N);

	albedo = vec4(0.039, 0.156, 0.47, roughness);
	normal = vec4(fn * 0.5 + 0.5, 0.1);
}