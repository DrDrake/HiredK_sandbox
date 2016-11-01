#version 400 core

uniform mat4 u_ViewProjMatrix;
uniform sampler2D s_Tex0;

uniform float u_RenderingType;

-- vs
layout(location = 0) in vec3 vs_Position;
layout(location = 1) in vec2 vs_TexCoord;
layout(location = 2) in vec4 vs_Color;
out vec2 fs_TexCoord;
out vec4 fs_Color;

void main()
{
	gl_Position = u_ViewProjMatrix * vec4(vs_Position, 1);
	fs_TexCoord = vs_TexCoord;
	fs_Color = vs_Color;
}

-- fs
layout(location = 0) out vec4 frag;
in vec2 fs_TexCoord;
in vec4 fs_Color;

void main()
{
	if (u_RenderingType == 1) // color only
	{
		frag = fs_Color;
		return;
	}
	
	if (u_RenderingType == 2) // textured
	{
		frag = texture2D(s_Tex0, fs_TexCoord) * fs_Color;
		return;
	}
	
	if (u_RenderingType == 3) // alpha mask
	{
		frag = fs_Color;
		frag.a = texture2D(s_Tex0, fs_TexCoord).a;
		return;
	}
}