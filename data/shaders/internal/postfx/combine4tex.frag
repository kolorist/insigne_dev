#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec2 o_TexCoord;

uniform mediump sampler2D iu_ColorTex0;
uniform mediump sampler2D iu_ColorTex1;
uniform mediump sampler2D iu_ColorTex2;
uniform mediump sampler2D iu_ColorTex3;

void main()
{
	mediump vec3 color0 = texture(iu_ColorTex0, o_TexCoord.st).rgb;
	mediump vec3 color1 = texture(iu_ColorTex1, o_TexCoord.st).rgb;
	mediump vec3 color2 = texture(iu_ColorTex2, o_TexCoord.st).rgb;
	mediump vec3 color3 = texture(iu_ColorTex3, o_TexCoord.st).rgb;

	mediump vec3 outColor = (color0 + color1 + color2 + color3) / 4.0;
	o_Color = vec4(outColor, 1.0);
}