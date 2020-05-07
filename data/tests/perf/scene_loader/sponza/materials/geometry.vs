#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in mediump vec3 l_Normal_L;
layout (location = 2) in mediump vec3 l_Tangent_L;
layout (location = 3) in mediump vec2 l_TexCoord;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_viewProjectionMatrix;
	mediump vec3 iu_cameraPos;
	mediump vec3 iu_sh[9];
};

out mediump mat3 v_TBN;
out mediump vec2 v_TexCoord;
out mediump vec3 v_ViewDir_W;

void main()
{
	v_TexCoord = l_TexCoord;
	mediump vec3 normalW = l_Normal_L;
	mediump vec3 tangentW = l_Tangent_L;
	mediump vec3 bitangentW = cross(normalW, tangentW);
	v_TBN = mat3(tangentW, bitangentW, normalW);
	vec3 posW = l_Position_L;
	v_ViewDir_W = normalize(iu_cameraPos - posW);
	gl_Position = iu_viewProjectionMatrix * vec4(posW, 1.0f);
}