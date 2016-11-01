#version 400 core

uniform sampler2D butterflySampler;
uniform sampler2DArray imgSampler;
uniform float pass;

-- vs
layout(location = 0) in vec2 vs_Position;
layout(location = 1) in vec2 vs_TexCoord;
out vec2 gs_TexCoord;

void main() {
    gl_Position = vec4(vs_Position, 0.0, 1.0);
	gs_TexCoord = vs_TexCoord;
}

-- gs
layout(triangles) in;
layout(triangle_strip, max_vertices = 15) out;
in vec2 gs_TexCoord[];
out vec2 fs_TexCoord;

void main() {
    for (int i = 0; i < 5; ++i)
	{
        gl_Layer = i;
        gl_PrimitiveID = i;
		
        gl_Position = gl_in[0].gl_Position;	
        fs_TexCoord = gs_TexCoord[0];
        EmitVertex();
		
        gl_Position = gl_in[1].gl_Position;
        fs_TexCoord = gs_TexCoord[1];
        EmitVertex();
		
        gl_Position = gl_in[2].gl_Position;
        fs_TexCoord = gs_TexCoord[2];
        EmitVertex();
		
        EndPrimitive();
    }
}

-- fs
layout(location = 0) out vec4 frag;
in vec2 fs_TexCoord;

vec4 fft2(int layer, vec2 i, vec2 w) {
    vec4 input1 = texture(imgSampler, vec3(fs_TexCoord.x, i.x, layer), 0.0);
    vec4 input2 = texture(imgSampler, vec3(fs_TexCoord.x, i.y, layer), 0.0);
    float res1x = w.x * input2.x - w.y * input2.y;
    float res1y = w.y * input2.x + w.x * input2.y;
    float res2x = w.x * input2.z - w.y * input2.w;
    float res2y = w.y * input2.z + w.x * input2.w;
    return input1 + vec4(res1x, res1y, res2x, res2y);
}

void main() {
    vec4 data = texture(butterflySampler, vec2(fs_TexCoord.y, pass), 0.0);
    vec2 i = data.xy;
    vec2 w = data.zw;

    frag = fft2(gl_PrimitiveID, i, w);
}