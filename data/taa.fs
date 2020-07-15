#version 300 es
layout (location = 0) out mediump vec4 o_Color;

uniform mediump sampler2D u_HistTex;
uniform mediump sampler2D u_MainTex;

in mediump vec2 v_TexCoord;

void main()
{
	mediump vec3 histColor = texture(u_HistTex, v_TexCoord).rgb;
	mediump vec3 mainColor = texture(u_MainTex, v_TexCoord).rgb;
	
	mediump vec3 color = mix(histColor, mainColor, 1.0f / 8.0f);
	//mediump vec3 color = abs(mainColor - histColor);
	//mediump vec3 color = mainColor + histColor;

	o_Color = vec4(color, 1.0f);
}