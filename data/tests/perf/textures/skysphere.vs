#version 300 es
layout (location = 0) in highp vec3 l_Position;
layout (location = 1) in mediump vec2 l_TexCoord;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_viewProjectionMatrix;
	highp vec3 iu_cameraPosition;
	mediump vec3 iu_sh[9];
};

out mediump vec3 v_SampleDir;

void main()
{
	v_SampleDir = normalize(l_Position);
	highp vec4 posC = iu_viewProjectionMatrix * vec4(l_Position + iu_cameraPosition, 1.0f);
	gl_Position = vec4(posC.xyz, posC.z);
}
