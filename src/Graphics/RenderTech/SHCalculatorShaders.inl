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

void main()
{
	mediump vec3 outColor = texture(u_Tex, v_Normal).rgb;
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
