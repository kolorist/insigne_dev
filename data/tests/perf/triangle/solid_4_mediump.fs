#version 300 es
layout (location = 0) out mediump vec4 o_Color;

in mediump vec4 v_Color0;
in mediump vec4 v_Color1;
in mediump vec4 v_Color2;
in mediump vec4 v_Color3;

void main()
{
	o_Color = v_Color0 + v_Color1 + v_Color2 + v_Color3;
}