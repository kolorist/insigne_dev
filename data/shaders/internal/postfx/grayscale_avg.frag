#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec2 o_TexCoord;

uniform mediump sampler2D iu_ColorTex0;

void main()
{
	mediump vec4 color = texture(iu_ColorTex0, o_TexCoord.st);
	mediump float avg = (color.r, color.g, color.b) / 3.0;
	o_Color = vec4(avg, avg, avg, 1.0);
}