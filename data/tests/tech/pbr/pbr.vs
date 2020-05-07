#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in highp vec3 l_Normal_L;
layout (location = 2) in mediump vec3 l_Tangent_L;
layout (location = 3) in mediump vec2 l_TexCoord;

layout(std140) uniform ub_Scene
{
	mediump vec3 iu_CameraPos;
	highp mat4 iu_viewProjectionMatrix;
	mediump vec3 iu_sh[9];
};

out highp vec3 v_Normal_W;
out mediump mat3 v_TBN;
out mediump vec3 v_ViewDir_W;
out mediump vec2 v_TexCoord;

void main()
{
	v_TexCoord = l_TexCoord;
	v_Normal_W = l_Normal_L;
	mediump vec3 tangentW = l_Tangent_L;
	mediump vec3 bitangentW = cross(v_Normal_W, tangentW);
	v_TBN = mat3(tangentW, bitangentW, v_Normal_W);
	vec3 posW = l_Position_L;
	v_ViewDir_W = normalize(iu_CameraPos - posW);
	gl_Position = iu_viewProjectionMatrix * vec4(posW, 1.0f);
}