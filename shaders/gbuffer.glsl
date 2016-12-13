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
out mat3 fs_TBN;

void main()
{	
	fs_Position = (u_ModelMatrix * vec4(vs_Position.xyz, 1)).xyz;
	gl_Position = u_ViewProjMatrix * vec4(fs_Position.xyz, 1);  
    fs_TexCoord = vs_TexCoord;
	
	vec3 n = u_NormalMatrix * vs_Normal;
    vec3 t = u_NormalMatrix * vs_Tangent;
	
	vec3 N = normalize(n);
	vec3 T = normalize(t - dot(t, N) * N);
	vec3 B = cross(N, T);
	
	fs_TBN = mat3(T, B, N);
}

-- fs
layout(location = 0) out vec4 albedo;
layout(location = 1) out vec4 normal;
in vec3 fs_Position;
in vec2 fs_TexCoord;
in mat3 fs_TBN;

void main()
{
	vec4 color = texture(s_Albedo, fs_TexCoord);
	
	vec4 n = texture(s_Normal, fs_TexCoord);
	n.xyz = normalize(n.xyz * 2.0 - 1.0);
	
	n.xyz = n.xyz * vec3(0.2f, 0.2f, 1.0); // temp smooth normal
	
	n.xyz = normalize(fs_TBN * n.xyz);
	
	albedo = vec4(color.rgb, color.a);
    normal = vec4(n.xyz * 0.5 + 0.5, n.a);
}