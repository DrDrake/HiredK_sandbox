#version 400 core

uniform mat4 u_ViewProjMatrix;
uniform mat4 u_ModelMatrix;

uniform sampler2D s_Albedo;

-- vs
layout(location = 0) in vec3 vs_Position;
layout(location = 1) in vec2 vs_TexCoord;
out vec2 fs_TexCoord;

void main()
{	
	gl_Position = u_ViewProjMatrix * u_ModelMatrix * vec4(vs_Position.xyz, 1);  
    fs_TexCoord = vs_TexCoord;
}

-- fs
layout(location = 0) out vec4 albedo;
in vec2 fs_TexCoord;

void main()
{
	vec4 color = texture(s_Albedo, fs_TexCoord);
	albedo = vec4(color.rgb * 0.2, 1);
}