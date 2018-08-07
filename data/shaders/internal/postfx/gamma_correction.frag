#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec2 o_TexCoord;

uniform mediump sampler2D iu_ColorTex0;

void main()
{
	mediump float gamma = 2.2;
	mediump vec3 orgColor = texture(iu_ColorTex0, o_TexCoord).rgb;

	// gamma correction
	mapped = pow(orgColor, vec3(1.0 / gamma));
	o_Color = vec4(mapped, 1.0);
}