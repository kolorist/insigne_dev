#version 300 es
layout (location = 0) in highp vec2 l_Position_L;
layout (location = 1) in mediump vec4 l_Color;

out lowp vec4 v_Color;

void main()
{
	v_Color = l_Color;
	gl_Position = vec4(l_Position_L, 0.0f, 1.0f);
}