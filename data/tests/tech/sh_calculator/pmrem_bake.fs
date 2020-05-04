#version 300 es
layout (location = 0) out highp vec3 o_Color;

in highp vec3 v_Position;

layout(std140) uniform ub_PrefilterConfigs
{
	highp float u_Roughness;
};

uniform samplerCube u_Tex;

const highp float PI = 3.14159265359;

highp float DistributionGGX(highp vec3 N, highp vec3 H, highp float roughness)
{
    highp float a = roughness*roughness;
    highp float a2 = a*a;
    highp float NdotH = max(dot(N, H), 0.0);
    highp float NdotH2 = NdotH*NdotH;

    highp float nom   = a2;
    highp float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

highp float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
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

void main()
{
    highp vec3 N = normalize(v_Position);
    highp vec3 R = N;
    highp vec3 V = R;

    const uint SAMPLE_COUNT = 1024u;
    highp float totalWeight = 0.0;
    highp vec3 prefilteredColor = vec3(0.0);
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        highp vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        highp vec3 H  = ImportanceSampleGGX(Xi, N, u_Roughness);
        highp vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        highp float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0)
        {
			// sample from the environment's mip level based on roughness/pdf
			highp float D   = DistributionGGX(N, H, u_Roughness);
			highp float NdotH = max(dot(N, H), 0.0);
			highp float HdotV = max(dot(H, V), 0.0);
			highp float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;

			highp float resolution = 256.0; // resolution of source cubemap (per face)
			highp float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);
			highp float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

			highp float mipLevel = u_Roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

			highp vec3 hdrColor = textureLod(u_Tex, L, mipLevel).rgb;
			prefilteredColor += hdrColor * NdotL;
			totalWeight      += NdotL;
		}
	}
	prefilteredColor = prefilteredColor / totalWeight;

	o_Color = prefilteredColor;
}