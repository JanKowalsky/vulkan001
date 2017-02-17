#ifndef SHADER_INFO_H
#define SHADER_INFO_H

#include <vector>
#include <vulkan.h>
#include "vulkan_math.h"

enum object_type
{
	object_type_sphere,
	object_type_box,
	object_type_cylinder,
	object_type_cone
};

struct material
{
	glm::vec3 ref; //reflectance rgb
	float op = 1.0f; //opacity <0;1>
};

struct light
{
	glm::vec3 color;
	float pad0;
	glm::vec3 pos;
	float pad1;
};

struct object
{
	glm::fmat4x4 W;
	material mat;
	object_type obj_type;
	glm::vec3 pad0;
};

struct
{
	glm::fmat4x4 invV;
	uint32_t image_index;
	float image_plane_width = 1920.0f;
	float image_plane_height = 1080.0f;
	float image_plane_distance = 2.4142135f;
	float rand[32];
} constants;

struct
{
	int num_rays = 4;
	int num_objects = 1;
	int num_lights = 1;
}spec_constants;

VkSpecializationMapEntry spec_map_entries[]
{
	{ 0, 0, sizeof(int) },
	{ 1, 4, sizeof(int) },
	{ 2, 8, sizeof(int) }
};

std::vector<object> objects;
std::vector<light> lights;

#endif //SHADER_INFO_H