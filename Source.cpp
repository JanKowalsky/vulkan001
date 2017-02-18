#include "engine.h"
#include <stdlib.h>
#include "myscene.h"

int main()
{
	system(R"($HOME/VulkanSDK/1.0.39.1/x86_64/bin/glslangValidator -V -e "main" /home/jankowalski/CodeliteWorkspaces/Vulkan/vulkan001/shader_code/vs.vert -o vs.spv)");
	system(R"($HOME/VulkanSDK/1.0.39.1/x86_64/bin/glslangValidator -V -e "main" /home/jankowalski/CodeliteWorkspaces/Vulkan/vulkan001/shader_code/fs.frag -o fs.spv)");
	
	auto ms = std::make_shared<MyScene>();
	VulkanEngine::get().setScene(ms);
	VulkanEngine::get().run();
	
	return 0;
}