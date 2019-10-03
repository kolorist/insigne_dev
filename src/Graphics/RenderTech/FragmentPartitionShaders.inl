namespace stone
{
namespace tech
{

static const_cstr s_VertexShaderCode = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_WVP;
};

out highp vec3 v_Pos_W;

void main()
{
	highp vec4 pos_W = vec4(l_Position_L, 1.0f);
	gl_Position = iu_WVP * pos_W;
	v_Pos_W = pos_W.xyz / pos_W.w;
}
)";

static const_cstr s_FragmentShaderCode = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_World
{
	highp vec4 iu_MinCorner;
	highp vec4 iu_Dimension;
};

in highp vec3 v_Pos_W;

void main()
{
	highp vec3 p = v_Pos_W - iu_MinCorner.xyz;
	highp vec3 cp = floor(p / iu_Dimension.xyz);
	mediump float c = (cp.x * 4.0f + cp.y * 16.0f + cp.z) / 64.0f;
	o_Color = vec4(vec3(c), 1.0f);
}
)";

}
}
