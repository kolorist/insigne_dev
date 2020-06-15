#version 300 es
layout (location = 0) out mediump vec3 o_Color;

uniform mediump sampler2D u_MainTex;

in mediump vec2 v_TexCoord;

const mediump float k_thresHold = 1.493477697f; // = 1.2 ^ 2.2
const mediump float k_thresholdKnee = 0.7467388483; // = k_thresHold * 0.5

void main()
{
	mediump vec3 mainColor = texture(u_MainTex, v_TexCoord).rgb;

	// thresholding
	mediump float brightness = max(mainColor.r, max(mainColor.g, mainColor.b));
	mediump float softness = clamp(brightness - k_thresHold + k_thresholdKnee, 0.0f, 2.0f * k_thresholdKnee);
	softness = (softness * softness) / (4.0f * k_thresholdKnee + 0.0001f);
	mediump float multiplier = max(brightness - k_thresHold, softness) / max(brightness, 0.0001f);
	mainColor *= multiplier;

	o_Color = mainColor;
}
