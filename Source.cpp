#include "engine.h"
#include <stdlib.h>
#include "myscene.h"

int main()
{
	//system(R"($HOME/VulkanSDK/1.0.39.0/x86_64/bin/glslangValidator -V -e "main" /home/jankowalski/CodeliteWorkspaces/Vulkan/RayTracer_001/shaders_code/cmp.comp -o compute_shader.spv)");
	
	auto ms = std::make_shared<MyScene>();
	VulkanEngine::get().setScene(ms);
	VulkanEngine::get().run();
	
	return 0;
}