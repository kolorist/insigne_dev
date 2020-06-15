#version 300 es
layout (location = 0) out mediump vec4 o_Color;

uniform mediump sampler2D u_MainTex;

in mediump vec2 v_TexCoord;

void main()
{
	o_Color = vec4(texture(u_MainTex, v_TexCoord).rgb, 1.0f);
}