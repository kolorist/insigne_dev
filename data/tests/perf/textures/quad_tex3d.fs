#version 300 es
layout (location = 0) out mediump vec4 o_Color;

uniform mediump sampler3D u_MainTex;

in mediump vec2 v_TexCoord;

void main()
{
	mediump vec3 mainColor = texture(u_MainTex, vec3(v_TexCoord, 1.0f)).rgb;
	mainColor /= vec3(255.0f, 255.0f, 255.0f);
	//mainColor = pow(mainColor, vec3(0.454545f));
	o_Color = vec4(mainColor, 1.0f);
}