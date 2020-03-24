namespace stone
{
namespace tech
{

// ---------------------------------------------

static const_cstr s_ProbeVS = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_WVP;
	mediump vec3 iu_SH[9];
};

out mediump vec3 v_Normal;

void main() {
	highp vec4 pos_W = vec4(l_Position_L, 1.0f);
	v_Normal = normalize(l_Position_L);
	gl_Position = iu_WVP * pos_W;
}
)";

// ---------------------------------------------

static const_cstr s_ProbeFS = R"(#version 300 es
layout (location = 0) out mediump vec3 o_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_WVP;
	mediump vec3 iu_SH[9];
};

in mediump vec3 v_Normal;

mediump vec3 eval_sh_irradiance(in mediump vec3 i_normal)
{
	const mediump float c0 = 0.2820947918f;		// sqrt(1.0 / (4.0 * pi))
	const mediump float c1 = 0.4886025119f;		// sqrt(3.0 / (4.0 * pi))
	const mediump float c2 = 1.092548431f;		// sqrt(15.0 / (4.0 * pi))
	const mediump float c3 = 0.3153915653f;		// sqrt(5.0 / (16.0 * pi))
	const mediump float c4 = 0.5462742153f;		// sqrt(15.0 / (16.0 * pi))

	const mediump float a0 = 3.141593f;
	const mediump float a1 = 2.094395f;
	const mediump float a2 = 0.785398f;

	//TODO: we can pre-multiply those above factors with SH coeffs before transfering them
	//to the GPU

	return
		a0 * c0 * iu_SH[0]

		- a1 * c1 * i_normal.x * iu_SH[1]
		+ a1 * c1 * i_normal.y * iu_SH[2]
		- a1 * c1 * i_normal.z * iu_SH[3]

		+ a2 * c2 * i_normal.z * i_normal.x * iu_SH[4]
		- a2 * c2 * i_normal.x * i_normal.y * iu_SH[5]
		+ a2 * c3 * (3.0 * i_normal.y * i_normal.y - 1.0) * iu_SH[6]
		- a2 * c2 * i_normal.y * i_normal.z * iu_SH[7]
		+ a2 * c4 * (i_normal.z * i_normal.z - i_normal.x * i_normal.x) * iu_SH[8];
}

void main()
{
	mediump vec3 shColor = eval_sh_irradiance(v_Normal);
	o_Color = shColor;
}
)";

// ---------------------------------------------

static const_cstr s_Preview_CubeMapHStripFS = R"(#version 300 es
layout (location = 0) out mediump vec3 o_Color;

uniform mediump samplerCube u_Tex;
in mediump vec3 v_Normal;

layout(std140) uniform ub_Preview
{
	mediump float iu_texLod;
};

void main()
{
	mediump vec3 outColor = textureLod(u_Tex, v_Normal, iu_texLod).rgb;
	o_Color = outColor;
}
)";

static const_cstr s_Preview_LatLongFS = R"(#version 300 es
layout (location = 0) out mediump vec3 o_Color;

uniform mediump sampler2D u_Tex;
in mediump vec3 v_Normal;

mediump vec3 textureLatLong(in sampler2D i_tex, in mediump vec3 i_sampleDir)
{
	mediump float phi = atan(i_sampleDir.x, i_sampleDir.z);
	mediump float theta = acos(i_sampleDir.y);

	mediump vec2 uv = vec2((3.14f + phi) * 0.15923f, theta * 0.31847f);
	return texture(i_tex, uv).rgb;
}

void main()
{
	mediump vec3 sampleDir = normalize(v_Normal);
	mediump vec3 outColor = textureLatLong(u_Tex, sampleDir);
	o_Color = outColor;
}
)";

static const_cstr s_Preview_LightProbeFS = R"(#version 300 es
layout (location = 0) out mediump vec3 o_Color;

uniform mediump sampler2D u_Tex;
in mediump vec3 v_Normal;

mediump vec3 textureLightprobe(in sampler2D i_tex, in mediump vec3 i_sampleDir)
{
	mediump float d = sqrt(i_sampleDir.x * i_sampleDir.x + i_sampleDir.y * i_sampleDir.y);
	mediump float r = 0.15924f * acos(i_sampleDir.z) / max(d, 0.001f);
	mediump vec2 uv = vec2(0.5f + i_sampleDir.x * r, 0.5f - i_sampleDir.y * r);
	return texture(i_tex, uv).rgb;
}

void main()
{
	mediump vec3 sampleDir = normalize(v_Normal);
	mediump vec3 outColor = textureLightprobe(u_Tex, sampleDir);
	o_Color = outColor;
}
)";

// ---------------------------------------------

static const_cstr s_PMREMFS = R"(layout (location = 0) out highp vec3 o_Color;

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
)";

static const_cstr s_PMREMVS = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_WVP;
	highp vec3 iu_CamPosition;
};

out highp vec3 v_Position;

void main()
{
	highp vec4 pos_W = iu_WVP * vec4(l_Position_L + iu_CamPosition, 1.0);
	gl_Position = pos_W.xyww;
	v_Position = l_Position_L;
}
)";

// ---------------------------------------------

static const_cstr s_ToneMapVS = R"(#version 300 es
layout (location = 0) in highp vec2 l_Position;
layout (location = 1) in mediump vec2 l_TexCoord;

out mediump vec2 v_TexCoord;

void main()
{
	v_TexCoord = l_TexCoord;
	gl_Position = vec4(l_Position, 0.0f, 1.0f);
}
)";

static const_cstr s_ToneMapFS = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

uniform mediump sampler2D u_Tex;

in mediump vec2 v_TexCoord;

void main()
{
	mediump float gamma = 2.2;
	mediump vec3 hdrColor = texture(u_Tex, v_TexCoord).rgb;
	
	// reinhard tone mapping
	mediump vec3 mapped = hdrColor / (hdrColor + vec3(1.0));
	// gamma correction
	mapped = pow(mapped, vec3(1.0 / gamma));
	o_Color = vec4(mapped, 1.0);
}
)";

}
}
