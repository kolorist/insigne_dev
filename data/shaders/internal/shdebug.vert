#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in mediump vec3 l_Normal_L;
layout (location = 2) in mediump vec2 l_TexCoord;

uniform highp mat4 iu_PerspectiveWVP;
uniform highp mat4 iu_TransformMat;

out mediump vec3 v_Normal_W;
out mediump vec2 v_TexCoord;

void main() {
	highp vec4 pos_W = iu_TransformMat * vec4(l_Position_L, 1.0f);
	mediump vec4 normal_W = iu_TransformMat * vec4(l_Normal_L, 0.0f);
	v_Normal_W = normalize(normal_W.xyz);
	v_TexCoord = l_TexCoord;
	gl_Position = iu_PerspectiveWVP * pos_W;
}
