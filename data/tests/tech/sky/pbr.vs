#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in highp vec3 l_Normal_L;
layout (location = 2) in mediump vec3 l_Tangent_L;
layout (location = 3) in mediump vec2 l_TexCoord;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_viewProjectionMatrix;
	highp mat4 iu_xformMatrix;
	mediump vec3 iu_CameraPos;
};

layout(std140) uniform ub_Configs
{
	mediump vec3 configs_camera;
	mediump vec3 configs_whitePoint;
	mediump vec3 configs_earthCenter;
	mediump vec3 configs_sunDirection;
	mediump vec2 configs_sunSize;
	mediump float configs_exposure;
	mediump float configs_lengthUnitInMeters;
};

out mediump mat3 v_TBN;
out mediump vec3 v_ViewDir_W;
out mediump vec2 v_TexCoord;
out highp vec3 v_Position;

void main()
{
	v_TexCoord = l_TexCoord;
	mediump vec3 normalW = (iu_xformMatrix * vec4(l_Normal_L, 0.0f)).xyz;
	mediump vec3 tangentW = (iu_xformMatrix * vec4(l_Tangent_L, 0.0f)).xyz;
	mediump vec3 bitangentW = cross(normalW, tangentW);
	v_TBN = mat3(tangentW, bitangentW, normalW);
	vec3 posW = (iu_xformMatrix * vec4(l_Position_L, 1.0f)).xyz;
	v_ViewDir_W = normalize(iu_CameraPos - posW);
	v_Position = posW - configs_earthCenter;
	gl_Position = iu_viewProjectionMatrix * vec4(posW, 1.0f);
}