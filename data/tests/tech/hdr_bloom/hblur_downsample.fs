#version 300 es
layout (location = 0) out mediump vec3 o_Color;

uniform mediump sampler2D u_MainTex;

in mediump vec2 v_TexCoord;

void main()
{
	mediump vec2 texelSize = 2.0f / vec2(textureSize(u_MainTex, 0));

	// 9-tap gaussian blur on the downsampled source
	mediump vec3 c0 = texture(u_MainTex, v_TexCoord - vec2(texelSize.x * 4.0f, 0.0f)).rgb;
	mediump vec3 c1 = texture(u_MainTex, v_TexCoord - vec2(texelSize.x * 3.0f, 0.0f)).rgb;
	mediump vec3 c2 = texture(u_MainTex, v_TexCoord - vec2(texelSize.x * 2.0f, 0.0f)).rgb;
	mediump vec3 c3 = texture(u_MainTex, v_TexCoord - vec2(texelSize.x * 1.0f, 0.0f)).rgb;
	mediump vec3 c4 = texture(u_MainTex, v_TexCoord								 ).rgb;
	mediump vec3 c5 = texture(u_MainTex, v_TexCoord + vec2(texelSize.x * 1.0f, 0.0f)).rgb;
	mediump vec3 c6 = texture(u_MainTex, v_TexCoord + vec2(texelSize.x * 2.0f, 0.0f)).rgb;
	mediump vec3 c7 = texture(u_MainTex, v_TexCoord + vec2(texelSize.x * 3.0f, 0.0f)).rgb;
	mediump vec3 c8 = texture(u_MainTex, v_TexCoord + vec2(texelSize.x * 4.0f, 0.0f)).rgb;

	mediump vec3 color = c0 * 0.01621622f + c1 * 0.05405405f + c2 * 0.12162162f + c3 * 0.19459459f
		+ c4 * 0.22702703f
		+ c5 * 0.19459459f + c6 * 0.12162162f + c7 * 0.05405405f + c8 * 0.01621622f;

	o_Color = color;
}
