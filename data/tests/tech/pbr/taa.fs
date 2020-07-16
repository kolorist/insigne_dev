#version 300 es
layout (location = 0) out mediump vec4 o_Color;

uniform mediump sampler2D u_HistTex;
uniform mediump sampler2D u_MainTex;

in mediump vec2 v_TexCoord;

void main()
{
	mediump vec3 histColor = texture(u_HistTex, v_TexCoord).rgb;
	mediump vec3 mainColor = texture(u_MainTex, v_TexCoord).rgb;

	// https://community.arm.com/developer/tools-software/graphics/b/blog/posts/temporal-anti-aliasing
	// reduce ghosting
	mediump vec3 nearColor0 = textureOffset(u_MainTex, v_TexCoord, ivec2(1, 0)).rgb;
	mediump vec3 nearColor1 = textureOffset(u_MainTex, v_TexCoord, ivec2(0, 1)).rgb;
	mediump vec3 nearColor2 = textureOffset(u_MainTex, v_TexCoord, ivec2(-1, 0)).rgb;
	mediump vec3 nearColor3 = textureOffset(u_MainTex, v_TexCoord, ivec2(0, -1)).rgb;

	mediump vec3 boxMin = min(mainColor, min(nearColor0, min(nearColor1, min(nearColor2, nearColor3))));
	mediump vec3 boxMax = max(mainColor, max(nearColor0, max(nearColor1, max(nearColor2, nearColor3))));

	histColor = clamp(histColor, boxMin, boxMax);

	mediump vec3 color = mix(histColor, mainColor, 1.0f / 8.0f);

	o_Color = vec4(color, 1.0f);
}