#version 300 es
layout (location = 0) out mediump vec4 o_Color;

in mediump vec4 v_Color;
in highp vec3 v_Normal;

void main()
{
	o_Color = vec4(abs(v_Normal), 1.0f);
}