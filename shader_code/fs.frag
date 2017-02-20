#version 450

layout(location=0) in vec2 tex_coord;

layout(location=0) out vec4 out_col;

void main()
{
	out_col = vec4(0.4f,0.6f,0.6f,1);
}