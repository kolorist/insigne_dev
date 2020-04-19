#version 300 es
layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_Material
{
	mediump vec3 iu_Color;
};

uniform mediump sampler2D u_Tex;

in mediump vec2 v_TexCoord;

void main()
{
	mediump vec3 color = texture(u_Tex, v_TexCoord).rgb;
	o_Color = vec4(color, 1.0f);
}