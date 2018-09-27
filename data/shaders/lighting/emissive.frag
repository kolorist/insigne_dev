#version 300 es

layout (location = 0) out mediump vec4 o_Color;

uniform mediump vec3 iu_BaseColor;
uniform mediump float iu_Strength;

layout(std140) uniform ub_TestUB
{
	vec4 inputColor;
};

void main()
{
	o_Color = vec4(iu_BaseColor, 1.0f) * ub_TestUB.inputColor * iu_Strength;
}
