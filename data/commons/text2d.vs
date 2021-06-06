#version 300 es
layout (location = 0) in highp vec2 l_Position_L;
layout (location = 1) in mediump vec4 l_Corner;
layout (location = 2) in mediump vec4 l_Color;

layout(std140) uniform ub_XForm
{
	highp mat4 iu_WVP;
};

out mediump vec2 v_TexCoord;
out mediump vec4 v_Color;

void main()
{
	v_Color = l_Color;
	v_TexCoord = l_Corner.zw;
	highp vec4 pos = vec4(l_Position_L + l_Corner.xy, 0.0f, 1.0f);
	gl_Position = pos;
}