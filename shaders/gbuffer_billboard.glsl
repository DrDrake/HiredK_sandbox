#version 330

uniform mat4 u_ViewProjMatrix;                                                                   
uniform vec3 u_CameraPosition;                                                            
uniform float u_BillboardSize;

uniform sampler2D s_AlbedoTex;

-- vs
layout (location = 0) in vec3 vs_Position;

void main()
{
	gl_Position = vec4(vs_Position, 1.0);
}

-- gs
layout(points) in;
layout(triangle_strip) out;
layout(max_vertices = 4) out;
out vec2 fs_TexCoord;

void main()                                                                         
{                                                                                   
    vec3 Pos = gl_in[0].gl_Position.xyz;
    vec3 toCamera = normalize(u_CameraPosition - Pos);
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = cross(toCamera, up) * u_BillboardSize;
	
	Pos.y -= (u_BillboardSize * 0.5);
    Pos -= (right * 0.5);
    gl_Position = u_ViewProjMatrix * vec4(Pos, 1.0);
    fs_TexCoord = vec2(0.0, 0.0);
    EmitVertex();
	
    Pos.y += u_BillboardSize;
    gl_Position = u_ViewProjMatrix * vec4(Pos, 1.0);
    fs_TexCoord = vec2(0.0, 1.0);
    EmitVertex();
	
    Pos.y -= u_BillboardSize;
    Pos += right;
    gl_Position = u_ViewProjMatrix * vec4(Pos, 1.0);
    fs_TexCoord = vec2(1.0, 0.0);
    EmitVertex();
	
    Pos.y += u_BillboardSize;
    gl_Position = u_ViewProjMatrix * vec4(Pos, 1.0);
    fs_TexCoord = vec2(1.0, 1.0);
    EmitVertex();
	
    EndPrimitive();
}

-- fs
layout(location = 0) out vec4 frag;
in vec2 fs_TexCoord;

void main()
{
	vec4 color = texture(s_AlbedoTex, fs_TexCoord);
    if (color.a <= 0.1) {           
        discard;                                                                    
    }
	
	frag = vec4(color.rgb * 50.0, color.a);
}