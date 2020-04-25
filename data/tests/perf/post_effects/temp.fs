#version 300 es
layout (location = 0) out mediump vec3 o_Color;

uniform mediump sampler2D u_MainTex;
uniform mediump sampler2D u_DepthTex;

in mediump vec2 v_TexCoord;

void main()
{
	mediump vec3 mainColor = texture2D(u_MainTex, v_TexCoord).rgb;
	mediump vec3 depthColor = texture2D(u_DepthTex, v_TexCoord).rrr;
	o_Color = mainColor + depthColor;
}