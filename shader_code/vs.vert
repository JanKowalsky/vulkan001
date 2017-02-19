#version 450

const uint res_x = 1920;
const uint res_y = 1080;
const float delta = 1.0f;
const float speed = 10.0f;

layout(set=0, binding=0, rgba32f) uniform imageBuffer vPos;

layout(location=0) out vec2 tex_coord;

void main()
{
	vec2 coord = vec2(gl_VertexIndex % res_x, gl_VertexIndex / res_x);
	tex_coord = vec2(coord.x / res_x, coord.y / res_y);
	
	vec4 currPos = imageLoad(vPos, gl_VertexIndex);
	vec4 destPos = vec4(-res_x/2 + coord.x*delta, res_y/2 - coord.y*delta, 0, 0);
	
	vec4 dir = normalize(destPos - currPos);
	gl_Position = currPos;
	
	imageStore(vPos, gl_VertexIndex, currPos + dir*speed);
}