#version 300 es
layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_Blit
{
	mediump vec3 iu_OverlayColor;
};

uniform mediump sampler2D u_MainTex;

in mediump vec2 v_TexCoord;

void main()
{
	mediump vec3 mainColor = texture2D(u_MainTex, v_TexCoord).rgb;
	mainColor *= iu_OverlayColor;
	o_Color = vec4(clamp(mainColor, vec3(0.0f), vec3(1.0f)), 1.0f);
}