#version 400 core

uniform sampler2D s_Tex0;
uniform sampler2D s_Tex1;

uniform mat4 u_ViewProjMatrix;
uniform mat4 u_ModelMatrix;
uniform mat3 u_NormalMatrix;

uniform float u_Timer;

-- vs
layout(location = 0) in vec3 vs_Position;
layout(location = 1) in vec2 vs_TexCoord;
layout(location = 2) in vec4 vs_Color;
layout(location = 3) in vec3 vs_Normal;
layout(location = 4) in vec3 vs_Tangent;
out vec2 fs_TexCoord;
out vec4 fs_Color;
out vec3 fs_Normal;
out vec3 fs_Tangent;

float hash( float n ) {
    return fract(sin(n)*43758.5453);
}

float noise( in vec2 x )
{
    vec2 p = floor(x);
    vec2 f = fract(x);
    f = f*f*(3.0-2.0*f);
    float n = p.x + p.y*57.0;
    float res = mix(mix( hash(n+  0.0), hash(n+  1.0),f.x), mix( hash(n+ 57.0), hash(n+ 58.0),f.x),f.y);
    return res;
}

float fbm( vec2 p )
{
    float f = 0.0;
    f += 0.50000*noise( p ); p = p*2.02;
    f += 0.03125*noise( p );
    return f/0.984375;
}

void main()
{
    vec4 p = u_ModelMatrix * vec4(vs_Position.xyz, 1);    
    float n = fbm(p.xy * u_Timer * 2.0) * 4.0 - 2.0;
    p = p + (vec4(n, 0, 0, 0) * vs_Color * 0.2);
    
	gl_Position = u_ViewProjMatrix * p;
    fs_TexCoord = vs_TexCoord;
    fs_Color = vs_Color;   
	fs_Normal = u_NormalMatrix * vs_Normal;
    fs_Tangent = u_NormalMatrix * vs_Tangent;
}

-- fs
layout(location = 0) out vec4 albedo;
layout(location = 1) out vec4 normal;
in vec2 fs_TexCoord;
in vec4 fs_Color;
in vec3 fs_Normal;
in vec3 fs_Tangent;

void main()
{
    vec3 N = normalize(fs_Normal);

	albedo = vec4(0, 0, 0, 1);
    normal = vec4(N * 0.5 + 0.5, 0);
}