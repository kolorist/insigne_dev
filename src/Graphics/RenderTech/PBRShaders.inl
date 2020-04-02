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

static const_cstr s_BRDF_IntegrationVS = R"(#version 300 es
layout (location = 0) in highp vec2 l_Position_L;
layout (location = 1) in highp vec2 l_TexCoord;

out highp vec2 v_TexCoord;

void main()
{
	v_TexCoord = l_TexCoord;
	gl_Position = vec4(l_Position_L, 0.0f, 1.0f);
})";

static const_cstr s_BRDF_IntegrationFS = R"(#version 300 es
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
)";

// ---------------------------------------------

static const_cstr s_PBRWithIBLVS = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in highp vec3 l_Normal_L;
layout (location = 2) in mediump vec2 l_TexCoord;
layout (location = 3) in highp vec3 l_Tangent_L;
layout (location = 4) in highp vec3 l_Binormal_L;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_XForm;
	highp mat4 iu_WVP;
	highp vec3 iu_CamPosition;
};

out highp vec3 v_PosW;
out highp mat3 v_TBN;
out mediump vec2 v_TexCoord;
out highp vec3 v_ViewDir_W;

void main()
{
	highp vec4 normal_W = iu_XForm * vec4(l_Normal_L, 0.0f);
	highp vec4 tangent_W = iu_XForm * vec4(l_Tangent_L, 0.0f);
	highp vec4 binormal_W = iu_XForm * vec4(l_Binormal_L, 0.0f);
	v_TexCoord = l_TexCoord;
	highp vec4 pos_W = iu_XForm * vec4(l_Position_L, 1.0f);
	v_ViewDir_W = normalize(iu_CamPosition.xyz - pos_W.xyz);
	v_PosW = pos_W.xyz;
	gl_Position = iu_WVP * pos_W;
	
	v_TBN = mat3(tangent_W.xyz, binormal_W.xyz, normal_W.xyz);
})";

static const_cstr s_PBRWithIBLFS = R"(#version 300 es

layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_Light
{
	mediump vec3 iu_LightDirection;
	mediump vec3 iu_LightIntensity;
	mediump int iu_PointLightsCount;
	mediump vec3 iu_PointLightPositions[4];
	mediump vec4 iu_PointLightAttributes[4];	// intensity (rgb) and attenuation factor (a)
};

layout(std140) uniform ub_SH
{
	mediump vec3 iu_SH[9];
};

in highp vec3 v_PosW;
in highp mat3 v_TBN;
in mediump vec2 v_TexCoord;
in highp vec3 v_ViewDir_W;

uniform sampler2D u_AlbedoTex;
uniform sampler2D u_NormalTex;
uniform sampler2D u_AttributeTex;				// R - roughness; G - metallic; B - emissive; A - emissionMask

uniform samplerCube u_SpecMap;
uniform samplerCube u_IrrMap;
uniform sampler2D u_SplitSumLUT;

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

// fresnel
// WARNING: this function can produce trash pixels color of (nan; nan; nan), which then propagate through
// the pipeline and cause massive artifacts (screen flickering for 3d scene)
// REASON: pow(x, y) result will be undefined if x <= 0 or y <= 0
mediump vec3 fresnel_schlick_v3(in mediump vec3 v, in mediump vec3 n, in mediump vec3 f0)
{
	mediump float VoN = max(dot(v, n), 0.0f);
	return (f0 + (1.0f - f0) * pow(1.0f - VoN, 5.0f));
}

mediump vec3 fresnel_spherical_gaussian(in mediump vec3 v, in mediump vec3 n, in mediump vec3 f0)
{
	mediump float VoN = max(dot(v, n), 0.0f);
	return (f0 + (1.0f - f0) * exp2((-5.55473f * VoN - 6.98316f) * VoN));
}

// for ibl
mediump vec3 fresnel_schlick_v3_roughness(in mediump vec3 v, in mediump vec3 n, in mediump vec3 f0, in mediump float roughness)
{
	mediump float VoN = max(dot(v, n), 0.0f);
	return (f0 + (max(vec3(1.0f - roughness), f0) - f0) * pow(1.0f - VoN, 5.0f));
}

// for ibl
mediump vec3 fresnel_spherical_gaussian_roughness(in mediump vec3 v, in mediump vec3 n, in mediump vec3 f0, in mediump float roughness)
{
	mediump float VoN = max(dot(v, n), 0.0f);
	return (f0 + (max(vec3(1.0f - roughness), f0) - f0) * exp2((-5.55473f * VoN - 6.98316f) * VoN));
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

mediump float visibility_implicit(in mediump vec3 l, in mediump vec3 v, in mediump vec3 n,
	in mediump vec3 h, in mediump float roughness)
{
	mediump float VoN = max(dot(v, n), 0.0f);
	mediump float LoN = max(dot(l, n), 0.0f);
	return (VoN * LoN);
}

// normal distribution function
mediump float ndf_ggx(in mediump vec3 n, in mediump vec3 h, in mediump float roughness)
{
	mediump float alpha = roughness * roughness;
	mediump float NoH = max(dot(n, h), 0.0f);
	mediump float alphaSq = alpha * alpha;
	mediump float pDenom = 1.0f + NoH * NoH * (alphaSq - 1.0f);
	mediump float denom = 3.14159f * pDenom * pDenom;
	return (alphaSq / denom);
}

#define eval_fresnel fresnel_spherical_gaussian
#define eval_ibl_fresnel fresnel_spherical_gaussian_roughness
#define eval_ndf ndf_ggx
#define eval_visibility visibility_ggx_epicgames

// BRDFs
mediump vec3 brdf_cook_torrance(in mediump vec3 l, in mediump vec3 v, in mediump vec3 h, in mediump vec3 n,
	in mediump float roughness, in mediump vec3 f0)
{
	mediump float alpha = roughness * roughness;
	mediump float LoN = max(dot(l, n), 0.0f);
	mediump float VoN = max(dot(v, n), 0.0f);

	mediump vec3 fresnel = eval_fresnel(v, n, f0);
	mediump float ndf = eval_ndf(n, h, alpha);
	mediump float visibility = eval_visibility(l, v, n, h, roughness);

	return (fresnel * ndf * visibility) / max(4.0f * LoN * VoN, 0.001f);
}

void main()
{
	mediump vec3 l = normalize(iu_LightDirection);	// points to the light source
	mediump vec3 v = normalize(v_ViewDir_W);		// points to the camera
	mediump vec3 h = normalize(l + v);
	mediump vec3 n = texture(u_NormalTex, v_TexCoord).rgb;
	n = normalize(n * 2.0f - 1.0f);
	n = normalize(v_TBN * n);
	mediump vec3 r = reflect(-v, n);

	mediump vec4 attrib = texture(u_AttributeTex, v_TexCoord).rgba;
	mediump float roughness = attrib.r;
	mediump float metallic = attrib.g;
	mediump vec3 baseColor = texture(u_AlbedoTex, v_TexCoord).rgb;

	mediump vec3 f0 = vec3(0.04f);
	f0 = mix(f0, baseColor, metallic);

	highp vec3 Lo = vec3(0.0f, 0.0f, 0.0f);
#if 0
	// PBR - directional light
	mediump float alpha = roughness * roughness;
	mediump float LoN = max(dot(l, n), 0.0f);
	mediump float VoN = max(dot(v, n), 0.0f);

	mediump vec3 fresnel = eval_fresnel(v, h, f0);
	mediump float ndf = eval_ndf(n, h, roughness);
	mediump float visibility = eval_visibility(l, v, n, h, roughness);
	mediump vec3 brdf = (fresnel * ndf * visibility) / max(4.0f * LoN * VoN, 0.001f);

	mediump vec3 ks = fresnel;
	mediump vec3 kd = vec3(1.0f) - ks;
	kd *= (1.0f - metallic);
	Lo = (kd * baseColor / 3.14159f + brdf) * iu_LightIntensity * LoN;
	
	// PBR - point light
	for (int i = 0; i < iu_PointLightsCount; i++)
	{
		mediump vec3 pl = normalize(iu_PointLightPositions[i] - v_PosW);
		mediump vec3 ph = normalize(pl + v);
		mediump float pDistance = length(iu_PointLightPositions[i]- v_PosW);
		mediump float pAttenuation = 
			1.0f +
			2.0f * iu_PointLightAttributes[i].a * pDistance +
			iu_PointLightAttributes[i].a * iu_PointLightAttributes[i].a * pDistance * pDistance;
		mediump vec3 pRadiance = iu_PointLightAttributes[i].rgb / pAttenuation;
		
		mediump float pLoN = max(dot(pl, n), 0.0f);
		mediump vec3 pFresnel = eval_fresnel(v, ph, f0);
		mediump float pNdf = eval_ndf(n, ph, roughness);
		mediump float pVisibility = eval_visibility(pl, v, n, ph, roughness);
		mediump vec3 pBrdf = (pFresnel * pNdf * pVisibility) / max(4.0f * pLoN * VoN, 0.001f);
		Lo += (kd * baseColor / 3.14159f + pBrdf) * pRadiance * pLoN;
	}
	
	// emission
	highp vec3 emissive = vec3(attrib.b * 10.0f);
	highp vec3 emissionColor = emissive * baseColor;
	Lo += emissionColor;
#endif
	
	// IBL
	mediump vec3 F_ibl = eval_ibl_fresnel(v, n, f0, roughness);
	mediump vec3 kS_ibl = F_ibl;
	mediump vec3 kD_ibl = vec3(1.0f) - kS_ibl;
	kD_ibl *= 1.0 - metallic;

	//mediump vec3 irradiance = texture(u_IrrMap, n).rgb;
	mediump vec3 irradiance = eval_sh_irradiance(n);
	mediump vec3 diffuseIBL = irradiance * baseColor;
	
	mediump vec3 prefilteredEnvColor = textureLod(u_SpecMap, r, roughness * 4.0f).rgb;
	mediump float nov = min(max(dot(n, v), 0.0f), 0.99);
	mediump vec2 precomputedEnvBRDF = texture(u_SplitSumLUT, vec2(nov, 1.0f - roughness)).rg;
	mediump vec3 specularIBL = prefilteredEnvColor * (F_ibl * precomputedEnvBRDF.x + precomputedEnvBRDF.y);
	
	mediump vec3 ambientIBL = kD_ibl * diffuseIBL + specularIBL;

	o_Color = vec4(Lo + ambientIBL, 1.0f);

	//o_Color = vec4(F_ibl, 1.0f);

	//o_Color = vec4(diffuseIBL, 1.0f);
	//o_Color = vec4(specularIBL, 1.0f);
	//o_Color = vec4(precomputedEnvBRDF, 0.0f, 1.0f);
})";

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
