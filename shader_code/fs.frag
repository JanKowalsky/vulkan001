#version 450

layout(set=0, binding=1) uniform sampler2D img;

layout(location=0) in vec2 tex_coord;

layout(location=0) out vec4 out_col;

void main()
{
	out_col = texture(img, tex_coord);
}