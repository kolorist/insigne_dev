#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec2 o_TexCoord;

uniform mediump sampler2D iu_ColorTex0;
uniform mediump float iu_PixelWidth;

void main()
{
	// 1 12 66 220 495 792 924 792 495 220 66 12 1
	mediump vec3 color01 = texture(iu_ColorTex0, o_TexCoord.st - vec2(iu_PixelWidth * 5.077, 0)).rgb;
	mediump vec3 color23 = texture(iu_ColorTex0, o_TexCoord.st - vec2(iu_PixelWidth * 3.231, 0)).rgb;
	mediump vec3 color34 = texture(iu_ColorTex0, o_TexCoord.st - vec2(iu_PixelWidth * 1.385, 0)).rgb;
	mediump vec3 color5 = texture(iu_ColorTex0, o_TexCoord.st).rgb;
	mediump vec3 color43 = texture(iu_ColorTex0, o_TexCoord.st + vec2(iu_PixelWidth * 5.077, 0)).rgb;
	mediump vec3 color32 = texture(iu_ColorTex0, o_TexCoord.st + vec2(iu_PixelWidth * 3.231, 0)).rgb;
	mediump vec3 color10 = texture(iu_ColorTex0, o_TexCoord.st + vec2(iu_PixelWidth * 1.385, 0)).rgb;

	mediump vec3 outColor = 0.003 * (color01 + color10) + 0.07 * (color23 + color32) + 0.314 * (color34 + color43) + 0.226 * color5;
	o_Color = vec4(outColor, 1.0);
}
