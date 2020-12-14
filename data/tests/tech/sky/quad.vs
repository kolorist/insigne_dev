#version 300 es
layout (location = 0) in highp vec2 l_Position;
layout (location = 1) in mediump vec2 l_TexCoord;

layout(std140) uniform ub_Scene
{
	highp mat4 model_from_view;
	highp mat4 view_from_clip;
};

out highp vec3 view_ray;

void main()
{
	view_ray = (model_from_view * vec4((view_from_clip * vec4(l_Position, 0.0f, 1.0f)).xyz, 0.0)).xyz;
	gl_Position = vec4(l_Position, 0.0f, 1.0f);
}
