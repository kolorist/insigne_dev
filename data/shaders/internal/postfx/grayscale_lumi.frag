#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec2 o_TexCoord;

uniform mediump sampler2D iu_ColorTex0;

void main()
{
	mediump vec4 color = texture(iu_ColorTex0, o_TexCoord.st);
	// HDTV luminance
	mediump float lumi = color.r * 0.2126 + color.g * 0.7152 + color.b * 0.0722;
	o_Color = vec4(lumi, lumi, lumi, 1.0);
}