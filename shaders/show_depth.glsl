#version 400 core

uniform sampler2DArray s_DepthTex;
uniform float u_Layer;

-- vs
layout(location = 0) in vec4 vs_Position;
layout(location = 1) in vec2 vs_TexCoord;
out vec2 fs_TexCoord;

void main()
{
	gl_Position = vs_Position;
	fs_TexCoord = vs_TexCoord;
}

-- fs
layout(location = 0) out vec4 frag;
in vec2 fs_TexCoord;

void main()
{
	frag = vec4(vec3(texture(s_DepthTex, vec3(fs_TexCoord, u_Layer)).r), 1);
}