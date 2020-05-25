#version 300 es
layout (location = 0) out highp vec2 o_Color;

in highp vec2 v_TexCoord;

const highp float PI = 3.14159265359;

highp float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

highp float GeometrySchlickGGX(highp float NdotV, highp float roughness)
{
    highp float a = roughness;
    highp float k = (a * a) / 2.0;

    highp float nom   = NdotV;
    highp float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
highp float GeometrySmith(highp vec3 N, highp vec3 V, highp vec3 L, highp float roughness)
{
    highp float NdotV = max(dot(N, V), 0.0);
    highp float NdotL = max(dot(N, L), 0.0);
    highp float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    highp float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

highp vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

highp vec3 ImportanceSampleGGX(highp vec2 Xi, highp vec3 N, highp float roughness)
{
    highp float a = roughness*roughness;

    highp float phi = 2.0 * PI * Xi.x;
    highp float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    highp float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

    // from spherical coordinates to cartesian coordinates
    highp vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space vector to world-space sample vector
    highp vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    highp vec3 tangent   = normalize(cross(up, N));
    highp vec3 bitangent = cross(N, tangent);

    highp vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

highp vec2 IntegrateBRDF(highp float NdotV, highp float roughness)
{
    highp vec3 V;
    V.x = sqrt(1.0 - NdotV*NdotV);
    V.y = 0.0;
    V.z = NdotV;

    highp float A = 0.0;
    highp float B = 0.0;

    highp vec3 N = vec3(0.0, 0.0, 1.0);

    const uint SAMPLE_COUNT = 1024u;
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        highp vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        highp vec3 H  = ImportanceSampleGGX(Xi, N, roughness);
        highp vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        highp float NdotL = max(L.z, 0.0);
        highp float NdotH = max(H.z, 0.0);
        highp float VdotH = max(dot(V, H), 0.0);

        if(NdotL > 0.0)
        {
            highp float G = GeometrySmith(N, V, L, roughness);
            highp float G_Vis = (G * VdotH) / (NdotH * NdotV);
            highp float Fc = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return vec2(A, B);
}

void main()
{
    highp vec2 integratedBRDF = IntegrateBRDF(v_TexCoord.x, v_TexCoord.y);
    o_Color = integratedBRDF;
}