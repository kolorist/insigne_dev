#version 300 es
layout (location = 0) out mediump vec3 o_Color;

uniform mediump samplerCube u_Tex;
in mediump vec3 v_Normal;

layout(std140) uniform ub_Preview
{
	mediump float iu_texLod;
};

void main()
{
	mediump vec3 outColor = textureLod(u_Tex, v_Normal, iu_texLod).rgb;
	o_Color = outColor;
}