#version 400 core

uniform sampler2D s_Tex0;

uniform vec4 u_Scale;
uniform vec4 u_Bias;

-- vs
layout(location = 0) in vec2 vs_Position;
layout(location = 1) in vec2 vs_TexCoord;
out vec2 fs_TexCoord;

void main()
{
	gl_Position = vec4(vs_Position, 0, 1);
	fs_TexCoord = vs_TexCoord;
}

-- fs
layout(location = 0) out vec4 frag;
in vec2 fs_TexCoord;

void main()
{
	frag = max(vec4(0.0), texture(s_Tex0, fs_TexCoord) + u_Bias) * u_Scale;
}