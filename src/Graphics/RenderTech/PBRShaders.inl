namespace stone
{
namespace tech
{

static const_cstr s_VertexShaderCode = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in mediump vec4 l_VertColor;

out vec4 o_VertColor;

void main()
{
	o_VertColor = l_VertColor;
	gl_Position = vec4(l_Position_L, 1.0f);
}
)";

static const_cstr s_FragmentShaderCode = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

in mediump vec4 o_VertColor;

void main()
{
	o_Color = o_VertColor;
}
)";

// ---------------------------------------------
// PBR shader without IBL
static const_cstr s_PBRVS = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in mediump vec3 l_Normal_L;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_XForm;
	highp mat4 iu_WVP;
	mediump vec4 iu_CameraPos;					// we only use .xyz (w is always 0)
};

out mediump vec3 v_Normal_W;
out mediump vec3 v_ViewDir_W;

void main() {
	highp vec4 pos_W = iu_XForm * vec4(l_Position_L, 1.0f);
	mediump vec4 normal_W = iu_XForm * vec4(l_Normal_L, 0.0f);
	v_Normal_W = normalize(normal_W.xyz);
	v_ViewDir_W = normalize(iu_CameraPos.xyz - pos_W.xyz);
	gl_Position = iu_WVP * pos_W;
}
)";

static const_cstr s_PBRFS = R"(#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec3 v_Normal_W;
in mediump vec3 v_ViewDir_W;

layout(std140) uniform ub_Light
{
	mediump vec4 iu_LightDirection;				// we only use .xyz (w is always 0)
	mediump vec4 iu_LightIntensity;				// we only use .xyz (w is always 0)
};

layout(std140) uniform ub_Surface
{
	mediump vec4 iu_BaseColor;					// we only use .xyz (w is always 0)
	mediump vec4 iu_Attributes;					// .x is metallic; .y is roughness; z, w is always 0
};

// fresnel
mediump vec3 fresnel_schlick_v3(in mediump vec3 v, in mediump vec3 n, in mediump vec3 f0)
{
	mediump float oneMinusVoN = max(1.0f - dot(v, n), 0.0f);
	return (f0 + (1.0f - f0) * pow(oneMinusVoN, 5.0f));
}

mediump float fresnel_spherical_gaussian(in mediump vec3 v, in mediump vec3 n, in mediump float f0)
{
	mediump float VoN = max(dot(v, n), 0.0f);
	return (f0 + (1.0f - f0) * exp2((-5.55473f * VoN - 6.98316f) * VoN));
}

// visibility
mediump float visibility_ggx_epicgames(in mediump vec3 l, in mediump vec3 v, in mediump vec3 n,
	in mediump vec3 h, in mediump float roughness)
{
	mediump float k = (roughness + 1.0f) * (roughness + 1.0f) / 8.0f;
	mediump float VoN = max(dot(v, n), 0.0f);
	mediump float LoN = max(dot(l, n), 0.0f);
	mediump float ggxL = LoN / (LoN * (1.0f - k) + k);
	mediump float ggxV = VoN / (VoN * (1.0f - k) + k);
	return (ggxL * ggxV);
}

// normal distribution function
mediump float ndf_ggx(in mediump vec3 n, in mediump vec3 h, in mediump float roughness)
{
	mediump float alpha = roughness * roughness;
	mediump float NoH = max(dot(n, h), 0.0f);
	mediump float alphaSq = alpha * alpha;
	return (alphaSq /
		(3.14159f * pow(1.0f + NoH * NoH * (alphaSq - 1.0f), 2.0f)));
}

// BRDFs
mediump float brdf_cook_torrance(in mediump vec3 l, in mediump vec3 v, in mediump vec3 h, in mediump vec3 n,
	in mediump float roughness, in mediump float f0)
{
	mediump float alpha = roughness * roughness;
	mediump float LoN = max(dot(l, n), 0.0f);
	mediump float VoN = max(dot(v, n), 0.0f);

	mediump float fresnel = fresnel_spherical_gaussian(v, n, f0);
	mediump float ndf = ndf_ggx(n, h, alpha);
	mediump float visibility = visibility_ggx_epicgames(l, v, n, h, roughness);

	return (fresnel * ndf * visibility) / max(4.0f * LoN * VoN, 0.001f);
}

void main()
{
	mediump vec3 l = normalize(iu_LightDirection.xyz);	// points to the light source
	mediump vec3 v = normalize(v_ViewDir_W);		// points to the camera
	mediump vec3 h = normalize(l + v);
	mediump vec3 n = normalize(v_Normal_W);
	mediump vec3 r = reflect(-v, n);

	mediump float roughness = iu_Attributes.y;
	mediump float metallic = iu_Attributes.x;
	mediump vec3 baseColor = iu_BaseColor.xyz;

	mediump vec3 f0 = vec3(0.04f);
	f0 = mix(f0, baseColor, metallic);

	// PBR
	mediump float alpha = roughness * roughness;
	mediump float LoN = max(dot(l, n), 0.0f);
	mediump float VoN = max(dot(v, n), 0.0f);

	mediump vec3 fresnel = fresnel_schlick_v3(v, h, f0);
	mediump float ndf = ndf_ggx(n, h, roughness);
	mediump float visibility = visibility_ggx_epicgames(l, v, n, h, roughness);
	mediump vec3 brdf = (fresnel * ndf * visibility) / max(4.0f * LoN * VoN, 0.001f);

	mediump vec3 ks = fresnel;
	mediump vec3 kd = vec3(1.0f) - ks;
	kd *= (1.0f - metallic);
	mediump vec3 Lo = (kd * baseColor / 3.14159f + brdf) * iu_LightIntensity.xyz * LoN;

	o_Color = vec4(Lo, 1.0f);
}
)";

// ---------------------------------------------

static const_cstr s_PMREMFS = R"(#version 300 es
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
