#version 400 core

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
    float dist = abs(fs_TexCoord.y);
    float z = fs_TexCoord.x + 0.03 * (1.0 - smoothstep(0.98, 1.0, dist));
    float blending = 1.0 - smoothstep(1.0, 2.0, dist);	
	frag = vec4(z, 0.0, 0.0, blending);
}