#version 300 es
layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_Material
{
	mediump vec4 iu_Color;
};

void main()
{
	o_Color = vec4(iu_Color);
}