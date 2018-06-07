#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec2 o_TexCoord;

uniform mediump sampler2D iu_ColorTex0;

void main()
{
	mediump vec3 outColor = texture(iu_ColorTex0, o_TexCoord.st).rgb;
	// gamma correktion
	outColor = pow(outColor, vec3(0.45f));
	o_Color = vec4(outColor, 1.0f);
}