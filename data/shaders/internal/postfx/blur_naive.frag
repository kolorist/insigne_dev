#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec2 o_TexCoord;

uniform mediump sampler2D iu_ColorTex0;
uniform mediump float iu_PixelWidth;
uniform mediump float iu_PixelHeight;

void main()
{
	mediump vec3 color00 = texture(iu_ColorTex0, o_TexCoord.st + vec2(-iu_PixelWidth * 2.0, -iu_PixelHeight * 2.0)).rgb;
	mediump vec3 color10 = texture(iu_ColorTex0, o_TexCoord.st + vec2(-iu_PixelWidth * 1.0, -iu_PixelHeight * 2.0)).rgb;
	mediump vec3 color20 = texture(iu_ColorTex0, o_TexCoord.st + vec2(0.0, -iu_PixelHeight * 2.0)).rgb;
	mediump vec3 color30 = texture(iu_ColorTex0, o_TexCoord.st + vec2(iu_PixelWidth * 1.0, -iu_PixelHeight * 2.0)).rgb;
	mediump vec3 color40 = texture(iu_ColorTex0, o_TexCoord.st + vec2(iu_PixelWidth * 2.0, -iu_PixelHeight * 2.0)).rgb;
	
	mediump vec3 color01 = texture(iu_ColorTex0, o_TexCoord.st + vec2(-iu_PixelWidth * 2.0, -iu_PixelHeight * 1.0)).rgb;
	mediump vec3 color11 = texture(iu_ColorTex0, o_TexCoord.st + vec2(-iu_PixelWidth * 1.0, -iu_PixelHeight * 1.0)).rgb;
	mediump vec3 color21 = texture(iu_ColorTex0, o_TexCoord.st + vec2(0.0, -iu_PixelHeight * 1.0)).rgb;
	mediump vec3 color31 = texture(iu_ColorTex0, o_TexCoord.st + vec2(iu_PixelWidth * 1.0, -iu_PixelHeight * 1.0)).rgb;
	mediump vec3 color41 = texture(iu_ColorTex0, o_TexCoord.st + vec2(iu_PixelWidth * 2.0, -iu_PixelHeight * 1.0)).rgb;
	
	mediump vec3 color02 = texture(iu_ColorTex0, o_TexCoord.st + vec2(-iu_PixelWidth * 2.0, 0.0)).rgb;
	mediump vec3 color12 = texture(iu_ColorTex0, o_TexCoord.st + vec2(-iu_PixelWidth * 1.0, 0.0)).rgb;
	mediump vec3 color22 = texture(iu_ColorTex0, o_TexCoord.st).rgb;
	mediump vec3 color32 = texture(iu_ColorTex0, o_TexCoord.st + vec2(iu_PixelWidth * 1.0, 0.0)).rgb;
	mediump vec3 color42 = texture(iu_ColorTex0, o_TexCoord.st + vec2(iu_PixelWidth * 2.0, 0.0)).rgb;
	
	mediump vec3 color03 = texture(iu_ColorTex0, o_TexCoord.st + vec2(-iu_PixelWidth * 2.0, iu_PixelHeight * 1.0)).rgb;
	mediump vec3 color13 = texture(iu_ColorTex0, o_TexCoord.st + vec2(-iu_PixelWidth * 1.0, iu_PixelHeight * 1.0)).rgb;
	mediump vec3 color23 = texture(iu_ColorTex0, o_TexCoord.st + vec2(0.0, iu_PixelHeight * 1.0)).rgb;
	mediump vec3 color33 = texture(iu_ColorTex0, o_TexCoord.st + vec2(iu_PixelWidth * 1.0, iu_PixelHeight * 1.0)).rgb;
	mediump vec3 color43 = texture(iu_ColorTex0, o_TexCoord.st + vec2(iu_PixelWidth * 2.0, iu_PixelHeight * 1.0)).rgb;
	
	mediump vec3 color04 = texture(iu_ColorTex0, o_TexCoord.st + vec2(-iu_PixelWidth * 2.0, iu_PixelHeight * 2.0)).rgb;
	mediump vec3 color14 = texture(iu_ColorTex0, o_TexCoord.st + vec2(-iu_PixelWidth * 1.0, iu_PixelHeight * 2.0)).rgb;
	mediump vec3 color24 = texture(iu_ColorTex0, o_TexCoord.st + vec2(0.0, iu_PixelHeight * 2.0)).rgb;
	mediump vec3 color34 = texture(iu_ColorTex0, o_TexCoord.st + vec2(iu_PixelWidth * 1.0, iu_PixelHeight * 2.0)).rgb;
	mediump vec3 color44 = texture(iu_ColorTex0, o_TexCoord.st + vec2(iu_PixelWidth * 2.0, iu_PixelHeight * 2.0)).rgb;

	mediump vec3 outColor = (
		color00 + color10 * 4.0 + color20 * 6.0 + color30 * 4.0 + color40 +
		color01 * 4.0 + color11 * 16.0 + color21 * 24.0 + color31 * 16.0 + color41 * 4.0 +
		color02 * 6.0 + color12 * 24.0 + color22 * 36.0 + color32 * 24.0 + color42 * 6.0 +
		color03 * 4.0 + color13 * 16.0 + color23 * 24.0 + color33 * 16.0 + color43 * 4.0 +
		color04 + color14 * 4.0 + color24 * 6.0 + color34 * 4.0 + color44
		) / 256.0;
	o_Color = vec4(outColor, 1.0);
}
