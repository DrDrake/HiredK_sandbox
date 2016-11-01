#version 400 core

uniform mat4 u_ViewProjMatrix;
uniform mat4 u_ModelMatrix;

uniform sampler2D s_Tex0;

-- vs
layout(location = 0) in vec3 vs_Position;
layout(location = 1) in vec3 vs_Normal;
layout(location = 2) in vec2 vs_TexCoord;
out vec3 fs_Normal;
out vec2 fs_TexCoord;

void main()
{	
	gl_Position = u_ViewProjMatrix * u_ModelMatrix * vec4(vs_Position.xyz, 1);
	fs_Normal = vs_Normal;
	fs_TexCoord = vs_TexCoord;
}

-- fs
layout(location = 0) out vec4 diffuse;
layout(location = 1) out vec4 geometric;
in vec3 fs_Normal;
in vec2 fs_TexCoord;

void main()
{
	diffuse = vec4(texture(s_Tex0, fs_TexCoord).rgb, 1);
	geometric = vec4(fs_Normal, 0.0);
}