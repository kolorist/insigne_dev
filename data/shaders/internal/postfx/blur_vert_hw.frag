#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec2 o_TexCoord;

uniform mediump sampler2D iu_ColorTex0;
uniform mediump float iu_PixelHeight;

void main()
{
	mediump vec3 color01 = texture(iu_ColorTex0, o_TexCoord.st - vec2(0, iu_PixelHeight * 5.077)).rgb;
	mediump vec3 color23 = texture(iu_ColorTex0, o_TexCoord.st - vec2(0, iu_PixelHeight * 3.231)).rgb;
	mediump vec3 color34 = texture(iu_ColorTex0, o_TexCoord.st - vec2(0, iu_PixelHeight * 1.385)).rgb;
	mediump vec3 color5 = texture(iu_ColorTex0, o_TexCoord.st).rgb;
	mediump vec3 color43 = texture(iu_ColorTex0, o_TexCoord.st + vec2(0, iu_PixelHeight * 5.077)).rgb;
	mediump vec3 color32 = texture(iu_ColorTex0, o_TexCoord.st + vec2(0, iu_PixelHeight * 3.231)).rgb;
	mediump vec3 color10 = texture(iu_ColorTex0, o_TexCoord.st + vec2(0, iu_PixelHeight * 1.385)).rgb;

	mediump vec3 outColor = 0.003 * (color01 + color10) + 0.07 * (color23 + color32) + 0.314 * (color34 + color43) + 0.226 * color5;
	o_Color = vec4(outColor, 1.0);
}
