#version 450

layout(location=0) in vec3 in_col;

void main()
{
	gl_Position = vec4(in_col, 1.0f);
}