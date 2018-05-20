#version 300 es

layout (location = 0) out mediump vec4 o_Color;

uniform mediump sampler2D iu_Tex;

in mediump vec2 o_TexCoord;
in mediump vec4 o_VertColor;

void main()
{
	o_Color = o_VertColor * texture(iu_Tex, o_TexCoord.st);
}