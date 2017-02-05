#version 400 core

uniform vec4 u_Offset;
uniform vec4 u_DeformOffset;
uniform vec4 u_Stroke;
uniform vec4 u_StrokeEnd;
uniform float u_Scale;

-- vs
layout(location = 0) in vec3 vs_Position;
out vec3 fs_Position;

void main()
{	
    vec2 uv = vs_Position.xy * u_Offset.xy + u_Offset.zw;
    fs_Position = vec3(uv * u_DeformOffset.z + u_DeformOffset.xy, 0.0);
	gl_Position = vec4(vs_Position.xy, 0.0, 1.0);
}

-- fs
layout(location = 0) out vec4 frag;
in vec3 fs_Position;

float segDist(vec2 a, vec2 b, vec2 p) {
    vec2 ab = b - a;
    vec2 ap = p - a;
    vec2 q = a + clamp(max(dot(ap, ab) / dot(ab, ab), 0.0), 0.0, 1.0) * ab;
    return length(p - q);
}

void main()
{
    float d = segDist(u_Stroke.xz, u_StrokeEnd.xz, fs_Position.xy) / u_Stroke.w;
    float value = 1.0 - smoothstep(0.0, 1.0, d);
	frag = vec4(value, 0, 0, 0) * u_Scale;
}