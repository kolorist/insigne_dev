namespace stone
{
namespace tech
{

static const_cstr s_AlbedoVS = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in highp vec3 l_Normal_L;
layout (location = 2) in mediump vec4 l_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_WVP;
};

out mediump vec4 v_Color;

void main()
{
	highp vec4 pos_W = vec4(l_Position_L, 1.0f);
	gl_Position = iu_WVP * pos_W;
	v_Color = l_Color;
}
)";

static const_cstr s_AlbedoFS = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

in mediump vec4 v_Color;

void main()
{
	o_Color = v_Color;
}
)";

//----------------------------------------------

static const_cstr s_ProbeVS = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_WVP;
};

layout(std140) uniform ub_ProbeData
{
	highp mat4 iu_XForm;
	mediump vec4 iu_Coeffs[9];
};

out mediump vec3 v_Normal;

void main() {
	highp vec4 pos_W = iu_XForm * vec4(l_Position_L, 1.0f);
	v_Normal = normalize(l_Position_L);
	gl_Position = iu_WVP * pos_W;
}
)";

static const_cstr s_ProbeFS = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_ProbeData
{
	highp mat4 iu_XForm;
	mediump vec4 iu_Coeffs[9];
};

in mediump vec3 v_Normal;

const mediump float c0 = 0.2820947918f;
const mediump float c1 = 0.4886025119f;
const mediump float c2 = 2.185096861f;	// sqrt(15/pi)
const mediump float c3 = 1.261566261f;	// sqrt(5/pi)

mediump vec3 evalSH(in mediump vec3 i_normal)
{
	return
		c0 * iu_Coeffs[0].xyz					// band 0

		- c1 * i_normal.y * iu_Coeffs[1].xyz * 0.667f
		+ c1 * i_normal.z * iu_Coeffs[2].xyz * 0.667f
		- c1 * i_normal.x * iu_Coeffs[3].xyz * 0.667f

		+ c2 * i_normal.x * i_normal.y * 0.5f * iu_Coeffs[4].xyz * 0.25f
		- c2 * i_normal.y * i_normal.z * 0.5f * iu_Coeffs[5].xyz * 0.25f
		+ c3 * (-1.0f + 3.0f * i_normal.z * i_normal.z) * 0.25f * iu_Coeffs[6].xyz * 0.25f
		- c2 * i_normal.x * i_normal.z * 0.5f * iu_Coeffs[7].xyz * 0.25f
		+ c2 * (i_normal.x * i_normal.x - i_normal.y * i_normal.y) * 0.25f * iu_Coeffs[8].xyz * 0.25f;
}

void main()
{
	mediump vec3 c = evalSH(v_Normal);
	o_Color = vec4(c, 1.0f);
}
)";

//----------------------------------------------

static const_cstr s_VertexShaderCode = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in highp vec3 l_Normal_L;
layout (location = 2) in mediump vec4 l_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_WVP;
};

out highp vec3 v_Pos_W;
out highp vec3 v_Normal;
out mediump vec3 v_Color;

void main()
{
	highp vec4 pos_W = vec4(l_Position_L, 1.0f);
	highp vec4 normal_W = iu_WVP * vec4(l_Normal_L, 0.0f);
	gl_Position = iu_WVP * pos_W;
	v_Pos_W = pos_W.xyz / pos_W.w;
	v_Normal = normal_W.xyz;
	v_Color = l_Color.xyz;
}
)";

static const_cstr s_FragmentShaderCode = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_World
{
	highp vec4 iu_MinCorner;
	highp vec4 iu_Dimension;
};

layout(std140) uniform ub_SHData
{
	mediump vec4 iu_Probes[576]; // 64 * 9 = 576
};

in highp vec3 v_Pos_W;
in highp vec3 v_Normal;
in mediump vec3 v_Color;

struct SH
{
	mediump vec3 CoEffs[9];
};

mediump vec3 evalSH(SH i_sh, in mediump vec3 i_normal)
{
	const mediump float c0 = 0.2820947918f;
	const mediump float c1 = 0.4886025119f;
	const mediump float c2 = 2.185096861f;	// sqrt(15/pi)
	const mediump float c3 = 1.261566261f;	// sqrt(5/pi)

	return
		c0 * i_sh.CoEffs[0]					// band 0

		- c1 * i_normal.y * i_sh.CoEffs[1] * 0.667f
		+ c1 * i_normal.z * i_sh.CoEffs[2] * 0.667f
		- c1 * i_normal.x * i_sh.CoEffs[3] * 0.667f

		+ c2 * i_normal.x * i_normal.y * 0.5f * i_sh.CoEffs[4] * 0.25f
		- c2 * i_normal.y * i_normal.z * 0.5f * i_sh.CoEffs[5] * 0.25f
		+ c3 * (-1.0f + 3.0f * i_normal.z * i_normal.z) * 0.25f * i_sh.CoEffs[6] * 0.25f
		- c2 * i_normal.x * i_normal.z * 0.5f * i_sh.CoEffs[7] * 0.25f
		+ c2 * (i_normal.x * i_normal.x - i_normal.y * i_normal.y) * 0.25f * i_sh.CoEffs[8] * 0.25f;
}

SH bilinearFetch(in highp vec3 i_cp0, in highp vec3 i_cp1, mediump float i_w)
{
	SH sh;
	mediump uint c0 = uint(dot(i_cp0, vec3(1.0f, 16.0f, 4.0f)));
	mediump uint c1 = uint(dot(i_cp1, vec3(1.0f, 16.0f, 4.0f)));
	sh.CoEffs[0u] = mix(iu_Probes[c0 * 9u     ].xyz, iu_Probes[c1 * 9u     ].xyz, i_w);
	sh.CoEffs[1u] = mix(iu_Probes[c0 * 9u + 1u].xyz, iu_Probes[c1 * 9u + 1u].xyz, i_w);
	sh.CoEffs[2u] = mix(iu_Probes[c0 * 9u + 2u].xyz, iu_Probes[c1 * 9u + 2u].xyz, i_w);
	sh.CoEffs[3u] = mix(iu_Probes[c0 * 9u + 3u].xyz, iu_Probes[c1 * 9u + 3u].xyz, i_w);
	sh.CoEffs[4u] = mix(iu_Probes[c0 * 9u + 4u].xyz, iu_Probes[c1 * 9u + 4u].xyz, i_w);
	sh.CoEffs[5u] = mix(iu_Probes[c0 * 9u + 5u].xyz, iu_Probes[c1 * 9u + 5u].xyz, i_w);
	sh.CoEffs[6u] = mix(iu_Probes[c0 * 9u + 6u].xyz, iu_Probes[c1 * 9u + 6u].xyz, i_w);
	sh.CoEffs[7u] = mix(iu_Probes[c0 * 9u + 7u].xyz, iu_Probes[c1 * 9u + 7u].xyz, i_w);
	sh.CoEffs[8u] = mix(iu_Probes[c0 * 9u + 8u].xyz, iu_Probes[c1 * 9u + 8u].xyz, i_w);
	return sh;
}

SH bilinearLerp(SH i_sh0, SH i_sh1, mediump float i_w)
{
	SH sh;
	sh.CoEffs[0u] = mix(i_sh0.CoEffs[0u], i_sh1.CoEffs[0u], i_w);
	sh.CoEffs[1u] = mix(i_sh0.CoEffs[1u], i_sh1.CoEffs[1u], i_w);
	sh.CoEffs[2u] = mix(i_sh0.CoEffs[2u], i_sh1.CoEffs[2u], i_w);
	sh.CoEffs[3u] = mix(i_sh0.CoEffs[3u], i_sh1.CoEffs[3u], i_w);
	sh.CoEffs[4u] = mix(i_sh0.CoEffs[4u], i_sh1.CoEffs[4u], i_w);
	sh.CoEffs[5u] = mix(i_sh0.CoEffs[5u], i_sh1.CoEffs[5u], i_w);
	sh.CoEffs[6u] = mix(i_sh0.CoEffs[6u], i_sh1.CoEffs[6u], i_w);
	sh.CoEffs[7u] = mix(i_sh0.CoEffs[7u], i_sh1.CoEffs[7u], i_w);
	sh.CoEffs[8u] = mix(i_sh0.CoEffs[8u], i_sh1.CoEffs[8u], i_w);
	return sh;
}

mediump vec3 computeSH(in highp vec3 i_fragPos, in highp vec3 i_fragNorm, in highp vec3 i_cp)
{
	mediump vec3 w = (i_fragPos - i_cp * iu_Dimension.xyz) / iu_Dimension.xyz;

	SH shx0 = bilinearFetch(i_cp						 , i_cp + vec3(1.0f, 0.0f, 0.0f), w.x);
	SH shx1 = bilinearFetch(i_cp + vec3(0.0f, 0.0f, 1.0f), i_cp + vec3(1.0f, 0.0f, 1.0f), w.x);
	SH shx2 = bilinearFetch(i_cp + vec3(0.0f, 1.0f, 0.0f), i_cp + vec3(1.0f, 1.0f, 0.0f), w.x);
	SH shx3 = bilinearFetch(i_cp + vec3(0.0f, 1.0f, 1.0f), i_cp + vec3(1.0f, 1.0f, 1.0f), w.x);

	SH shz0 = bilinearLerp(shx0, shx1, w.z);
	SH shz1 = bilinearLerp(shx2, shx3, w.z);

	SH sh = bilinearLerp(shz0, shz1, w.y);

	return evalSH(sh, i_fragNorm);
}

void main()
{
	highp vec3 p = v_Pos_W - iu_MinCorner.xyz;
	highp vec3 cp = floor(p / iu_Dimension.xyz);

	mediump vec3 shColor = computeSH(p, vec3(-v_Normal.x, v_Normal.y, v_Normal.z), cp);

	o_Color = vec4(shColor * v_Color, 1.0f);
	//o_Color = vec4(shColor, 1.0f);
}
)";

//----------------------------------------------

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
