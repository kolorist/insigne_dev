#version 300 es
layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_Material
{
	mediump vec3 iu_Color;
};

in mediump vec2 v_TexCoord;

uniform mediump sampler2D u_AlbedoTex;

void main()
{
	mediump vec3 color = texture(u_AlbedoTex, v_TexCoord).rgb;
	o_Color = vec4(color, 1.0f);
}