#version 300 es
layout (location = 0) out mediump vec3 o_Color;

uniform mediump sampler2D u_BigTex;
uniform mediump sampler2D u_SmallTex;

in mediump vec2 v_TexCoord;

const mediump float k_scatter = 0.7f;

void main()
{
	mediump vec3 bigMipColor = texture(u_BigTex, v_TexCoord).rgb;
	mediump vec3 smallMipColor = texture(u_SmallTex, v_TexCoord).rgb;

	o_Color = mix(bigMipColor, smallMipColor, k_scatter);
}
