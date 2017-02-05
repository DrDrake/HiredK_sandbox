#version 400 core

#define PARTICLE_TYPE_LAUNCHER 0.0f
#define PARTICLE_TYPE_SHELL 1.0f
#define PARTICLE_TYPE_SECONDARY_SHELL 2.0f

-- vs
layout (location = 0) in float vs_Type;
layout (location = 1) in vec3 vs_Position;
layout (location = 2) in vec3 vs_Velocity;
layout (location = 3) in float vs_Age;

uniform vec3 u_EmitterOrigin;

out float gs_Type0;
out vec3 gs_Position0;
out vec3 gs_Velocity0;
out float gs_Age0;

void main()
{
    gs_Type0 = vs_Type;
    gs_Position0 = (vs_Type == PARTICLE_TYPE_LAUNCHER) ? u_EmitterOrigin : vs_Position;
    gs_Velocity0 = vs_Velocity;
    gs_Age0 = vs_Age;
}

-- gs
layout(points) in;
layout(points) out;
layout(max_vertices = 50) out;

in float gs_Type0[];
in vec3 gs_Position0[];
in vec3 gs_Velocity0[];
in float gs_Age0[];

out float gs_Type1;
out vec3 gs_Position1;
out vec3 gs_Velocity1;
out float gs_Age1;

uniform sampler1D s_RandomTex;

uniform float u_Delta;
uniform float u_Timer;
uniform float u_LauncherLifetime;
uniform float u_ShellLifetime;
uniform float u_SecondaryShellLifetime;

vec3 GetRandomDir(float TexCoord)
{
     vec3 Dir = texture(s_RandomTex, TexCoord).xyz * 2.0 - 1.0;
     return Dir;
}

void main()
{
    float Age = gs_Age0[0] + u_Delta;
	
    if (gs_Type0[0] == PARTICLE_TYPE_LAUNCHER)
	{
        if (Age >= u_LauncherLifetime)
		{
            gs_Type1 = PARTICLE_TYPE_SHELL;
            gs_Position1 = gs_Position0[0];
			
            vec3 Dir = GetRandomDir(u_Timer);
            Dir.y = max(Dir.y, 0.5);
			
            gs_Velocity1 = normalize(Dir) / 20;
            gs_Age1 = 0.0;
            EmitVertex();
            EndPrimitive();
            Age = 0.0;
        }
		
        gs_Type1 = PARTICLE_TYPE_LAUNCHER;
        gs_Position1 = gs_Position0[0];
        gs_Velocity1 = gs_Velocity0[0];
        gs_Age1 = Age;
        EmitVertex();
        EndPrimitive();
    }
    else
	{ 
        float t1 = gs_Age0[0];
        float t2 = Age;
		
        vec3 DeltaP = u_Delta * gs_Velocity0[0] * 100.0;
        vec3 DeltaV = vec3(u_Delta) * (0.0, -9.81, 0.0) * 100.0;
		
        if (gs_Type0[0] == PARTICLE_TYPE_SHELL)
		{
	        if (Age < u_ShellLifetime)
			{
	            gs_Type1 = PARTICLE_TYPE_SHELL;
	            gs_Position1 = gs_Position0[0] + DeltaP;
	            gs_Velocity1 = gs_Velocity0[0] + DeltaV;
	            gs_Age1 = Age;
	            EmitVertex();
	            EndPrimitive();
	        }
            else
			{
                for (int i = 0 ; i < 32; i++)
				{
                     gs_Type1 = PARTICLE_TYPE_SECONDARY_SHELL;
                     gs_Position1 = gs_Position0[0];
                     vec3 Dir = GetRandomDir(((u_Timer * 1000) + i) / 1000);
                     gs_Velocity1 = normalize(Dir) / 20;
                     gs_Age1 = 0.0f;
                     EmitVertex();
                     EndPrimitive();
                }
            }
        }
        else
		{
            if (Age < u_SecondaryShellLifetime)
			{
                gs_Type1 = PARTICLE_TYPE_SECONDARY_SHELL;
                gs_Position1 = gs_Position0[0] + DeltaP;
                gs_Velocity1 = gs_Velocity0[0] + DeltaV;
                gs_Age1 = Age;
                EmitVertex();
                EndPrimitive();
            }
        }
    }
}