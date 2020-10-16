#version 300 es
layout (location = 0) in highp vec2 l_Position_L;
layout (location = 1) in mediump vec4 l_Color;

out highp vec4 v_Color0;
out highp vec4 v_Color1;
out highp vec4 v_Color2;
out highp vec4 v_Color3;

void main()
{
	v_Color0 = vec4(l_Color.r, 0.0f, 0.0f, 0.0f);
	v_Color1 = vec4(0.0f, l_Color.g, 0.0f, 0.0f);
	v_Color2 = vec4(0.0f, 0.0f, l_Color.b, 0.0f);
	v_Color3 = vec4(0.0f, 0.0f, 0.0f, l_Color.a);
	gl_Position = vec4(l_Position_L, 0.0f, 1.0f);
}