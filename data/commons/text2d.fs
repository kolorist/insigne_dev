#version 300 es
layout (location = 0) out mediump vec4 o_Color;

in mediump vec2 v_TexCoord;
in mediump vec4 v_Color;

uniform mediump sampler2D u_Tex;

void main()
{
	o_Color = mix(vec4(0.3f, 0.4f, 0.5f, 0.5f), v_Color, texture(u_Tex, v_TexCoord).r);
    //o_Color = vec4(v_Color.rgb, texture(u_Tex, v_TexCoord).r);
}