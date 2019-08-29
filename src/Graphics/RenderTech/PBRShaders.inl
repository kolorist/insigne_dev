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
	mediump float VoN = max(dot(v, n), 0.0f);
	return (f0 + (1.0f - f0) * pow(1.0f - VoN, 5.0f));
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
