#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec4 v_Color;

void main()
{
	o_Color = v_Color;
}
