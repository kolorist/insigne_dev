#version 300 es
layout (location = 0) out mediump vec4 o_Color;

in mediump vec2 v_TexCoord;

void main()
{
	o_Color = vec4(2.0f, 0.0f, 0.0f, 1.0f);
}