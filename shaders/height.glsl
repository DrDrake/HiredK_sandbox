#version 400 core

const int BORDER = 2;

uniform sampler2D _PermTable2D;
uniform sampler2D _Gradient3D;

uniform vec4 _TileWSD;
uniform sampler2D _CoarseLevelSampler; 
uniform vec4 _CoarseLevelOSL; 

uniform float _Amp;
uniform vec4 _Offset;
uniform mat4 _LocalToWorld;

uniform float _Frequency;

const mat4 slopexMatrix[4] = mat4[4](
	mat4(
		0.0, 0.0, 0.0, 0.0,
		1.0, 0.0,-1.0, 0.0,
		0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0
	),
	mat4(
		0.0, 0.0, 0.0, 0.0,
	    0.5, 0.5,-0.5,-0.5,
	    0.0, 0.0, 0.0, 0.0,
	    0.0, 0.0, 0.0, 0.0
	),
	mat4(
		0.0, 0.0, 0.0, 0.0,
	    0.5, 0.0,-0.5, 0.0,
	    0.5, 0.0,-0.5, 0.0,
	    0.0, 0.0, 0.0, 0.0
	),
	mat4(
		0.0, 0.0, 0.0, 0.0,
	    0.25, 0.25, -0.25, -0.25,
	    0.25, 0.25, -0.25, -0.25,
	    0.0, 0.0, 0.0, 0.0
	)
);

const mat4 slopeyMatrix[4] = mat4[4](
	mat4(
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0,
		0.0,-1.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0
	),
	mat4(
		0.0, 0.5, 0.5, 0.0,
		0.0, 0.0, 0.0, 0.0,
		0.0,-0.5,-0.5, 0.0,
		0.0, 0.0, 0.0, 0.0
	),
	mat4(
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0,-0.5, 0.0, 0.0,
		0.0,-0.5, 0.0, 0.0
	),
	mat4(
		0.0, 0.25, 0.25, 0.0,
		0.0, 0.25, 0.25, 0.0,
		0.0, -0.25, -0.25, 0.0,
		0.0, -0.25, -0.25, 0.0
	)
);

const mat4 curvatureMatrix[4] = mat4[4](
	mat4(
		0.0, -1.0, 0.0, 0.0,
			 -1.0, 4.0, -1.0, 0.0,
			 0.0, -1.0, 0.0, 0.0,
			 0.0, 0.0, 0.0, 0.0
	),
	mat4(
		0.0, -0.5, -0.5, 0.0,
			 -0.5, 1.5, 1.5, -0.5,
			 0.0, -0.5, -0.5, 0.0,
			 0.0, 0.0, 0.0, 0.0
	),
	mat4(
		0.0, -0.5, 0.0, 0.0,
			 -0.5, 1.5, -0.5, 0.0,
			 -0.5, 1.5, -0.5, 0.0,
			 0.0, -0.5, 0.0, 0.0
	),
	mat4(
		0.0, -0.25, -0.25, 0.0,
			 -0.25, 0.5, 0.5, -0.25,
			 -0.25, 0.5, 0.5, -0.25,
			 0.0, -0.25, -0.25, 0.0
	)
);

const mat4 upsampleMatrix[4] = mat4[4](
	mat4(
		0.0, 0.0, 0.0, 0.0,
			 0.0, 1.0, 0.0, 0.0,
			 0.0, 0.0, 0.0, 0.0,
			 0.0, 0.0, 0.0, 0.0
	),
	mat4(
		0.0, 0.0, 0.0, 0.0,
			 -1.0/16.0, 9.0/16.0, 9.0/16.0, -1.0/16.0,
			 0.0, 0.0, 0.0, 0.0,
			 0.0, 0.0, 0.0, 0.0
	),
	mat4(
		0.0, -1.0/16.0, 0.0, 0.0,
			 0.0, 9.0/16.0, 0.0, 0.0,
			 0.0, 9.0/16.0, 0.0, 0.0,
			 0.0, -1.0/16.0, 0.0, 0.0
	),
	mat4(
		1.0/256.0, -9.0/256.0, -9.0/256.0, 1.0/256.0,
			 -9.0/256.0, 81.0/256.0, 81.0/256.0, -9.0/256.0,
			 -9.0/256.0, 81.0/256.0, 81.0/256.0, -9.0/256.0,
			 1.0/256.0, -9.0/256.0, -9.0/256.0, 1.0/256.0
	)
);

-- vs
layout(location = 0) in vec4 vs_Position;
layout(location = 1) in vec2 vs_TexCoord;
out vec2 fs_UV;
out vec2 fs_ST;

void main() {	
	gl_Position = vs_Position;
	fs_UV = vs_TexCoord;
	fs_ST = vs_TexCoord * _TileWSD.x;
}

-- fs
in vec2 fs_UV;
in vec2 fs_ST;
out vec4 frag;

float mdot(mat4 a, mat4 b) {
    return dot(a[0], b[0]) + dot(a[1], b[1]) + dot(a[2], b[2]) + dot(a[3], b[3]);
}

vec4 perm2d(vec2 uv)
{
	return textureLod(_PermTable2D, uv, 0);
}

float gradperm(float x, vec3 p)
{
	vec3 g = textureLod(_Gradient3D, vec2(x, 0), 0).rgb *2.0 - 1.0;
	return dot(g, p);
}

vec3 fade(vec3 t) {
	return t * t * t * (t * (t * 6 - 15) + 10); // new curve
}

float inoise(vec3 p)
{
	vec3 P = mod(floor(p), 256.0);
  	p -= floor(p);
	vec3 f = fade(p);
	
	P = P / 256.0;
	const float one = 1.0 / 256.0;
	
    // HASH COORDINATES OF THE 8 CUBE CORNERS
	vec4 AA = perm2d(P.xy) + P.z;
 
	// AND ADD BLENDED RESULTS FROM 8 CORNERS OF CUBE
  	return mix( mix( mix( gradperm(AA.x, p ),  
                             gradperm(AA.z, p + vec3(-1, 0, 0) ), f.x),
                       mix( gradperm(AA.y, p + vec3(0, -1, 0) ),
                             gradperm(AA.w, p + vec3(-1, -1, 0) ), f.x), f.y),
                             
                 mix( mix( gradperm(AA.x+one, p + vec3(0, 0, -1) ),
                             gradperm(AA.z+one, p + vec3(-1, 0, -1) ), f.x),
                       mix( gradperm(AA.y+one, p + vec3(0, -1, -1) ),
                             gradperm(AA.w+one, p + vec3(-1, -1, -1) ), f.x), f.y), f.z);
}

void main() {
	vec2 p_uv = floor(fs_ST) * 0.5;
	vec2 uv = (p_uv - fract(p_uv) + vec2(0.5,0.5)) * _CoarseLevelOSL.z + _CoarseLevelOSL.xy;
	
	float zf = 0;
	
	mat4 cz = mat4(
		textureLod(_CoarseLevelSampler, uv + vec2(0.0, 0.0) *  _CoarseLevelOSL.z, 0).x,
		textureLod(_CoarseLevelSampler, uv + vec2(1.0, 0.0) *  _CoarseLevelOSL.z, 0).x,
		textureLod(_CoarseLevelSampler, uv + vec2(2.0, 0.0) *  _CoarseLevelOSL.z, 0).x,
		textureLod(_CoarseLevelSampler, uv + vec2(3.0, 0.0) *  _CoarseLevelOSL.z, 0).x,
		textureLod(_CoarseLevelSampler, uv + vec2(0.0, 1.0) *  _CoarseLevelOSL.z, 0).x,
		textureLod(_CoarseLevelSampler, uv + vec2(1.0, 1.0) *  _CoarseLevelOSL.z, 0).x,
		textureLod(_CoarseLevelSampler, uv + vec2(2.0, 1.0) *  _CoarseLevelOSL.z, 0).x,
		textureLod(_CoarseLevelSampler, uv + vec2(3.0, 1.0) *  _CoarseLevelOSL.z, 0).x,
		textureLod(_CoarseLevelSampler, uv + vec2(0.0, 2.0) *  _CoarseLevelOSL.z, 0).x,
		textureLod(_CoarseLevelSampler, uv + vec2(1.0, 2.0) *  _CoarseLevelOSL.z, 0).x,
		textureLod(_CoarseLevelSampler, uv + vec2(2.0, 2.0) *  _CoarseLevelOSL.z, 0).x,
		textureLod(_CoarseLevelSampler, uv + vec2(3.0, 2.0) *  _CoarseLevelOSL.z, 0).x,
		textureLod(_CoarseLevelSampler, uv + vec2(0.0, 3.0) *  _CoarseLevelOSL.z, 0).x,
		textureLod(_CoarseLevelSampler, uv + vec2(1.0, 3.0) *  _CoarseLevelOSL.z, 0).x,
		textureLod(_CoarseLevelSampler, uv + vec2(2.0, 3.0) *  _CoarseLevelOSL.z, 0).x,
		textureLod(_CoarseLevelSampler, uv + vec2(3.0, 3.0) *  _CoarseLevelOSL.z, 0).x
	);
	
	int i = int(dot(fract(p_uv), vec2(2.0, 4.0)));
	vec3 n = vec3(mdot(cz, slopexMatrix[i]), mdot(cz, slopeyMatrix[i]), 2.0 * _TileWSD.y);
	float slope = length(n.xy) / n.z;
	float curvature = mdot(cz, curvatureMatrix[i]) / _TileWSD.y;
	float noiseAmp = max(clamp(4.0 * curvature, 0.0, 1.5), clamp(2.0 * slope - 0.5, 0.1, 4.0));
	
	float u = (0.5+BORDER) / (_TileWSD.x-1-BORDER*2);
	vec2 vert = fs_UV * (1.0+u*2.0) - u;
	vert = vert * _Offset.z + _Offset.xy;
	
	vec3 P = vec3(vert, _Offset.w);
	mat3 LTW = mat3(_LocalToWorld);
	vec3 p = normalize(LTW * P).xyz;
	
	float noise = inoise(p * _Frequency);
	
	if (_Amp < 0.0) {
		zf -= _Amp * noise;
	}
	else {
		zf += noiseAmp * _Amp * noise;
	}
	
	float zc = zf;
	if (_CoarseLevelOSL.x != -1.0) 
	{
		zf = zf + mdot(cz, upsampleMatrix[i]);

		vec2 ij = floor(fs_ST - vec2(BORDER,BORDER));
		vec4 uvc = vec4(BORDER + 0.5,BORDER + 0.5,BORDER + 0.5,BORDER + 0.5);
		uvc += _TileWSD.z * floor((ij / (2.0 * _TileWSD.z)).xyxy + vec4(0.5, 0.0, 0.0, 0.5));
		
		float zc1 = textureLod(_CoarseLevelSampler, uvc.xy * _CoarseLevelOSL.z + _CoarseLevelOSL.xy, 0.0).x;
		float zc3 = textureLod(_CoarseLevelSampler, uvc.zw * _CoarseLevelOSL.z + _CoarseLevelOSL.xy, 0.0).x;
		
		zc = (zc1 + zc3) * 0.5;
	}
	
	frag = vec4(zf, zc, 0.0, 1.0);
}