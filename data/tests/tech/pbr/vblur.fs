#version 300 es
layout (location = 0) out mediump vec3 o_Color;

uniform mediump sampler2D u_MainTex;

in mediump vec2 v_TexCoord;

void main()
{
	mediump vec2 texelSize = 1.0f / vec2(textureSize(u_MainTex, 0));

	// Optimized bilinear 5-tap gaussian on the same-sized source (9-tap equivalent)
	mediump vec3 c0 = texture(u_MainTex, v_TexCoord - vec2(0.0f, texelSize.y * 3.23076923f)).rgb;
	mediump vec3 c1 = texture(u_MainTex, v_TexCoord - vec2(0.0f, texelSize.y * 1.38461538f)).rgb;
	mediump vec3 c2 = texture(u_MainTex, v_TexCoord                                      	).rgb;
	mediump vec3 c3 = texture(u_MainTex, v_TexCoord + vec2(0.0f, texelSize.y * 1.38461538f)).rgb;
	mediump vec3 c4 = texture(u_MainTex, v_TexCoord + vec2(0.0f, texelSize.y * 3.23076923f)).rgb;

	mediump vec3 color = c0 * 0.07027027f + c1 * 0.31621622f
		+ c2 * 0.22702703f
		+ c3 * 0.31621622f + c4 * 0.07027027f;

	o_Color = color;
}
