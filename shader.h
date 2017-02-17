#pragma once
#include <vulkan.h>
#include <string>

class Shader
{
public:
	Shader(std::string);
	~Shader();
	
	VkShaderModule getModule();
	
private:
	VkShaderModule m_module;
};