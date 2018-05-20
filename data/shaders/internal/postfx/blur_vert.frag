#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec2 o_TexCoord;

uniform mediump sampler2D iu_ColorTex0;
uniform mediump float iu_PixelHeight;

void main()
{
	mediump vec3 color0 = texture(iu_ColorTex0, o_TexCoord.st - vec2(0, iu_PixelHeight * 2.0)).rgb;
	mediump vec3 color1 = texture(iu_ColorTex0, o_TexCoord.st - vec2(0, iu_PixelHeight * 1.0)).rgb;
	mediump vec3 color2 = texture(iu_ColorTex0, o_TexCoord.st).rgb;
	mediump vec3 color3 = texture(iu_ColorTex0, o_TexCoord.st + vec2(0, iu_PixelHeight * 1.0)).rgb;
	mediump vec3 color4 = texture(iu_ColorTex0, o_TexCoord.st + vec2(0, iu_PixelHeight * 2.0)).rgb;

	mediump vec3 outColor = (color0 + color1 * 4.0 + color2 * 6.0 + color3 * 4.0 + color4) / 16.0;
	o_Color = vec4(outColor, 1.0);
}
