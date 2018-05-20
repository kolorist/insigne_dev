#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in mediump vec3 l_Normal_L;

uniform highp mat4 iu_PerspectiveWVP;
uniform highp mat4 iu_TransformMat;
uniform mediump vec3 iu_CameraPos;

out mediump vec3 v_Normal_W;
out mediump vec3 v_ViewDir_W;

void main() {
	highp vec4 pos_W = iu_TransformMat * vec4(l_Position_L, 1.0f);
	mediump vec4 normal_W = iu_TransformMat * vec4(l_Normal_L, 0.0f);
	v_Normal_W = normalize(normal_W.xyz);
	v_ViewDir_W = -normalize(pos_W.xyz - iu_CameraPos);
	gl_Position = iu_PerspectiveWVP * pos_W;
}