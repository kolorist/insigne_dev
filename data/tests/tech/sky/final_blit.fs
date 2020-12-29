#version 300 es
layout (location = 0) out mediump vec4 o_Color;

uniform mediump sampler2D u_MainTex;

in mediump vec2 v_TexCoord;

void main()
{
	mediump vec3 mainColor = texture(u_MainTex, v_TexCoord).rgb;
	mediump vec3 tonemapColor = pow(vec3(1.0) - exp(-mainColor), vec3(1.0 / 2.2));
	o_Color = vec4(tonemapColor, 1.0f);
}