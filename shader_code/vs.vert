#version 450

const float delta = 1.0f;

layout(push_constant) uniform pushConstants {
    layout(row_major)mat4x4 viewProj;
	float dt;
	float speed;
	uint res_x;
	uint res_y;
} pc;

layout(set=0, binding=0, rgba32f) uniform imageBuffer vPos;

layout(location=0) out vec2 tex_coord;

void main()
{
	float rX = float(pc.res_x);
	float rY = float(pc.res_y);
	
	uvec2 coord = uvec2(gl_VertexIndex % int(pc.res_x), gl_VertexIndex / int(pc.res_x));
	tex_coord = vec2(float(coord.x) / rX, float(coord.y) / rY);
	
	vec3 currPos = imageLoad(vPos, gl_VertexIndex).xyz;
	vec3 destPos = vec3(-rX/2 + float(coord.x)*delta, rY/2 - float(coord.y)*delta, 0);
	
	vec3 dir = destPos - currPos;
	float dist = length(dir);
	
	//if(dist >= pc.speed*pc.dt)
	if(dist >= 0.001)
	{
		dir /= dist;
		currPos +=  dir*pc.speed*pc.dt;
		imageStore(vPos, gl_VertexIndex, vec4(currPos, 0));
	}
	
	gl_Position = vec4(currPos, 1.0f) * pc.viewProj;
	gl_Position.y *= -1;
}