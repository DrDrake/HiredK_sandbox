#version 400 core

#define M_PI 3.14159265

uniform sampler2D spectrum_1_2_Sampler;
uniform sampler2D spectrum_3_4_Sampler;

uniform int FFT_SIZE;
uniform vec4 GRID_SIZES;

uniform float N_SLOPE_VARIANCE;
uniform float slopeVarianceDelta;
uniform float c;

-- vs
layout(location = 0) in vec2 vs_Position;
layout(location = 1) in vec2 vs_TexCoord;
out vec2 fs_TexCoord;

void main() {
    gl_Position = vec4(vs_Position, 0.0, 1.0);
	fs_TexCoord = vs_TexCoord;
}

-- fs
layout(location = 0) out float frag;
in vec2 fs_TexCoord;

float getSlopeVariance(vec2 k, float A, float B, float C, vec2 spectrumSample) {
    float w = 1.0 - exp(A * k.x * k.x + B * k.x * k.y + C * k.y * k.y);
    vec2 kw = k * w;
    return dot(kw, kw) * dot(spectrumSample, spectrumSample) * 2.0;
}

void main() {
    const float SCALE = 10.0;
    float a = floor(fs_TexCoord.x * N_SLOPE_VARIANCE);
    float b = floor(fs_TexCoord.y * N_SLOPE_VARIANCE);
    float A = pow(a / (N_SLOPE_VARIANCE - 1.0), 4.0) * SCALE;
    float C = pow(c / (N_SLOPE_VARIANCE - 1.0), 4.0) * SCALE;
    float B = (2.0 * b / (N_SLOPE_VARIANCE - 1.0) - 1.0) * sqrt(A * C);
    A = -0.5 * A;
    B = - B;
    C = -0.5 * C;

    float slopeVariance = slopeVarianceDelta;
    for (int y = 0; y < FFT_SIZE; ++y) {
        for (int x = 0; x < FFT_SIZE; ++x) {
            int i = x >= FFT_SIZE / 2 ? x - FFT_SIZE : x;
            int j = y >= FFT_SIZE / 2 ? y - FFT_SIZE : y;
            vec2 k = 2.0 * M_PI * vec2(i, j);

            vec4 spectrum12 = texture(spectrum_1_2_Sampler, vec2(float(x) + 0.5, float(y) + 0.5) / float(FFT_SIZE));
            vec4 spectrum34 = texture(spectrum_3_4_Sampler, vec2(float(x) + 0.5, float(y) + 0.5) / float(FFT_SIZE));

            slopeVariance += getSlopeVariance(k / GRID_SIZES.x, A, B, C, spectrum12.xy);
            slopeVariance += getSlopeVariance(k / GRID_SIZES.y, A, B, C, spectrum12.zw);
            slopeVariance += getSlopeVariance(k / GRID_SIZES.z, A, B, C, spectrum34.xy);
            slopeVariance += getSlopeVariance(k / GRID_SIZES.w, A, B, C, spectrum34.zw);
        }
    }
	
    frag = slopeVariance;
}