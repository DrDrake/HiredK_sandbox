#version 400 core

uniform sampler2D s_Tex0; // features
uniform sampler2D s_Tex1; // dirt
uniform sampler2D s_Tex2; // star
uniform mat4 u_LensStarMatrix;

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
	vec4 lensMod = texture(s_Tex1, fs_TexCoord);
	lensMod += texture(s_Tex2, (u_LensStarMatrix * vec4(fs_TexCoord, 0.0, 1.0)).xy);	
	frag = texture(s_Tex0, fs_TexCoord) * lensMod * 0.25;
}