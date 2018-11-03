#include "GIShader.h"

namespace stone {

namespace gi {

const_cstr g_FinalBlitVS = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in highp vec2 l_TexCoord;

out mediump vec2 v_TexCoord;

void main() {
	v_TexCoord = l_TexCoord;
	gl_Position = vec4(l_Position_L, 1.0f);
}
)";
const_cstr g_FinalBlitFS = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

in mediump vec2 v_TexCoord;

uniform mediump sampler2D iu_Tex;

void main() {
	mediump vec3 color = texture(iu_Tex, v_TexCoord).rgb;
	o_Color = vec4(color, 1.0f);
	//o_Color = vec4(v_TexCoord.x, v_TexCoord.y, 0.0f, 1.0f);
}
)";

const_cstr g_ShadowVS = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_XForm;
	highp mat4 iu_WVP;
};

void main() {
	highp vec4 pos_W = iu_XForm * vec4(l_Position_L, 1.0f);
	gl_Position = iu_WVP * pos_W;
}
)";
const_cstr g_ShadowFS = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

void main()
{
	o_Color = vec4(1.0f);
	// nothing :)
}
)";

const_cstr g_SurfaceVS = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in highp vec3 l_Normal_L;
layout (location = 2) in mediump vec4 l_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_XForm;
	highp mat4 iu_WVP;
	highp vec4 iu_CameraPos;
};

layout(std140) uniform ub_LightScene
{
	highp mat4 iu_LightXForm;
	highp mat4 iu_LightWVP;
};

layout(std140) uniform ub_Light
{
	highp vec4 iu_LightDirection;
	mediump vec4 iu_LightColor;
	mediump vec4 iu_LightRadiance;
};

out mediump vec4 v_VertexColor;
out highp vec3 v_Normal;
out highp vec3 v_LightSpacePos;
out highp vec3 v_ViewDir;

void main() {
	highp vec4 pos_W = iu_XForm * vec4(l_Position_L, 1.0f);
	v_VertexColor = l_Color;
	v_Normal = l_Normal_L;
	v_ViewDir = normalize(iu_CameraPos.xyz - pos_W.xyz);
	highp vec4 pos_LS = iu_LightWVP * iu_LightXForm * vec4(l_Position_L, 1.0f);
	v_LightSpacePos = pos_LS.xyz / pos_LS.w; // perspective division, no need for orthographic anyway
	gl_Position = iu_WVP * pos_W;
}
)";
const_cstr g_SurfaceFS = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_Light
{
	highp vec4 iu_LightDirection;
	mediump vec4 iu_LightColor;
	mediump vec4 iu_LightRadiance;
};

uniform highp sampler2D iu_ShadowMap;

in mediump vec4 v_VertexColor;
in highp vec3 v_Normal;
in highp vec3 v_LightSpacePos;
in highp vec3 v_ViewDir;

void main()
{
	highp vec3 n = normalize(v_Normal);
	highp vec3 l = normalize(iu_LightDirection.xyz);
	highp vec3 v = normalize(v_ViewDir);
	highp vec3 h = normalize(l + v);

	mediump float nol = max(dot(n, l), 0.0f);
	mediump float noh = max(dot(n, h), 0.0f);

	mediump vec3 r = normalize(- l - 2.0f * dot(n, -l) * n);
	mediump float rov = max(dot(r, v), 0.0f);
	mediump vec3 diff = v_VertexColor.xyz * iu_LightRadiance.xyz * nol;
	mediump vec3 spec = v_VertexColor.xyz * iu_LightRadiance.xyz * pow(noh, 90.0f);
	mediump vec3 color = diff + spec;

	// shadow
	highp vec2 smUV = (v_LightSpacePos.xy + vec2(1.0f)) / vec2(2.0f);
	highp float ld = texture(iu_ShadowMap, smUV).r;
	highp float sld = v_LightSpacePos.z * 0.5f + 0.5f;
	mediump float shadowMask = 1.0f - step(0.0002f, sld - ld);

	//o_Color = vec4(color * shadowMask, 1.0f);
	o_Color = vec4(v_VertexColor);
}
)";

const_cstr g_DebugSHProbeVS = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in mediump vec4 l_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_XForm;
	highp mat4 iu_WVP;
	highp vec4 iu_CameraPos;
};

out mediump vec3 v_Normal;

void main() {
	highp vec4 pos_W = iu_XForm * vec4(l_Position_L, 1.0f);
	v_Normal = normalize(l_Position_L);
	gl_Position = iu_WVP * pos_W;
}
)";
const_cstr g_DebugSHProbeFS = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_SHData
{
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

}

}
