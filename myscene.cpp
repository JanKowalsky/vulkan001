#include "myscene.h"
#include "shader.h"
#include <iostream>

enum SemNames{s_acquire_image, s_submit, num_sems};
enum FenNames{f_submit, num_fences};

MyScene::MyScene()
{
	initialize();
}

MyScene::~MyScene()
{
	destroy();
}

void MyScene::initialize()
{
	vkGetDeviceQueue(VulkanEngine::get().getDevice(), VulkanEngine::get().getQueueFamilyIndexGeneral(), 0, &m_queue);
	
	initSynchronizationObjects();
	initCommandBuffers();
	initSurfaceDependentObjects();
}

void MyScene::initSurfaceDependentObjects()
{
	initRenderTargets();
	initGraphicsPipeline();
}

void MyScene::destroySurfaceDependentObjects()
{
	VkDevice d = VulkanEngine::get().getDevice();
	
	if (m_graphics_pipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(d, m_graphics_pipeline, VK_NULL_HANDLE);
		m_graphics_pipeline = VK_NULL_HANDLE;
	}
	
	for(auto& rt : m_render_targets)
	{		
		if(rt.framebuffer != VK_NULL_HANDLE)
		{
			vkDestroyFramebuffer(d, rt.framebuffer, VK_NULL_HANDLE);
			rt.framebuffer = VK_NULL_HANDLE;
		}
	}
	
	if (m_render_pass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(d, m_render_pass, VK_NULL_HANDLE);
		m_render_pass = VK_NULL_HANDLE;
	}
}

void MyScene::initSynchronizationObjects()
{
	//fences---------------
	VkFenceCreateInfo fence_create_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, NULL, VK_FENCE_CREATE_SIGNALED_BIT};
	m_fences.resize(num_fences);
	for(auto& f : m_fences)
	{
		vkCreateFence(VulkanEngine::get().getDevice(), &fence_create_info, VK_NULL_HANDLE, &f);
	}
	
	//semaphores-----------
	VkSemaphoreCreateInfo sem_create_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, NULL, 0};
	m_semaphores.resize(num_sems);
	for(auto& s : m_semaphores)
	{
		vkCreateSemaphore(VulkanEngine::get().getDevice(), &sem_create_info, VK_NULL_HANDLE, &s);
	}
}

void MyScene::destroySynchronizationObjects()
{
	VkDevice d = VulkanEngine::get().getDevice();
	
	for(auto& f : m_fences)
	{
		vkDestroyFence(d, f, VK_NULL_HANDLE);
		f = VK_NULL_HANDLE;
	}
	
	for(auto& s : m_semaphores)
	{
		if (s != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(d, s, VK_NULL_HANDLE);
			s = VK_NULL_HANDLE;
		}
	}
}

void MyScene::initCommandBuffers()
{
	VkCommandPoolCreateInfo command_pool_create_info{};
	command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_create_info.pNext = NULL;
	command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	command_pool_create_info.queueFamilyIndex = VulkanEngine::get().getQueueFamilyIndexGeneral();
	
	vkCreateCommandPool(VulkanEngine::get().getDevice(), &command_pool_create_info, VK_NULL_HANDLE, &m_command_pool);
	
	/*create a command buffer for each swapchain image*/
	m_command_buffers.resize(VulkanEngine::get().getSwapchainImages().size());
	
	VkCommandBufferAllocateInfo command_buffer_allocate_info{};
	command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.pNext = NULL;
	command_buffer_allocate_info.commandPool = m_command_pool;
	command_buffer_allocate_info.commandBufferCount = m_command_buffers.size();
	command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	
	vkAllocateCommandBuffers(VulkanEngine::get().getDevice(), &command_buffer_allocate_info, m_command_buffers.data());
}

void MyScene::initRenderTargets()
{
	VkAttachmentDescription at_desc{};
	at_desc.flags = 0;
	at_desc.format = VulkanEngine::get().getSurfaceFormat();
	at_desc.samples = VK_SAMPLE_COUNT_1_BIT;
	at_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	at_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	at_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	at_desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	
	VkAttachmentReference col_at_ref[] =
	{
		{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}
	};
	
	VkSubpassDescription sub_desc{};
	sub_desc.flags = 0;
	sub_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	sub_desc.inputAttachmentCount = 0;
	sub_desc.pInputAttachments = NULL;
	sub_desc.colorAttachmentCount = 1;
	sub_desc.pColorAttachments = col_at_ref;
	sub_desc.pResolveAttachments = NULL;
	sub_desc.pDepthStencilAttachment = NULL;
	sub_desc.preserveAttachmentCount = 0;
	sub_desc.pPreserveAttachments = NULL;
	
	VkRenderPassCreateInfo render_pass_create_info{};
	render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.pNext = NULL;
	render_pass_create_info.flags = 0;
	render_pass_create_info.attachmentCount = 1;
	render_pass_create_info.pAttachments = &at_desc;
	render_pass_create_info.subpassCount = 1;
	render_pass_create_info.pSubpasses = &sub_desc;
	render_pass_create_info.dependencyCount = 0;
	render_pass_create_info.pDependencies = NULL;
	
	vkCreateRenderPass(VulkanEngine::get().getDevice(), &render_pass_create_info, VK_NULL_HANDLE, &m_render_pass);
	
	/*---Creating framebuffers---*/
	/*Create a set of identical framebuffers, one for each swapchain image*/
	
	VkFramebufferCreateInfo frambuffer_create_info{};
	frambuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frambuffer_create_info.pNext = NULL;
	frambuffer_create_info.flags = 0;
	frambuffer_create_info.renderPass = m_render_pass;
	frambuffer_create_info.attachmentCount = 1;
	frambuffer_create_info.width = VulkanEngine::get().getSurfaceExtent().width;
	frambuffer_create_info.height = VulkanEngine::get().getSurfaceExtent().height;
	frambuffer_create_info.layers = 1;
	
	/*Specify clear values used for each framebuffer*/
	std::vector<VkClearValue> cv = {
			{VkClearColorValue{1, 0, 0, 0}}
		};
	
	/*Specify render pass begin info fields, which are identical for each framebuffer,
	specify the variant ones later, per framebuffer*/
	
	VkRenderPassBeginInfo render_pass_begin_info{};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.pNext = NULL;
	render_pass_begin_info.renderPass = m_render_pass;
	render_pass_begin_info.renderArea.extent = VulkanEngine::get().getSurfaceExtent();
	render_pass_begin_info.renderArea.offset = VkOffset2D{0,0};
	render_pass_begin_info.clearValueCount = cv.size();
	
	/*Creting a render target structure for each swapchain image*/
	m_render_targets.resize(VulkanEngine::get().getSwapchainImageViews().size());
	
	/*For each render target*/
	for(size_t i = 0; i < m_render_targets.size(); i++)
	{
		/*Specify the attachment for each framebuffer as a different swapchain image view*/
		frambuffer_create_info.pAttachments = &VulkanEngine::get().getSwapchainImageViews()[i];
		/*Create a framebuffer for given render target*/
		vkCreateFramebuffer(VulkanEngine::get().getDevice(), &frambuffer_create_info, VK_NULL_HANDLE, &m_render_targets[i].framebuffer);
		
		/*Set the same clear values for each render target*/
		m_render_targets[i].clear_values = cv;
		
		/*Set the render pass for given render target*/
		m_render_targets[i].render_pass = m_render_pass;
		
		/*Set the render pass begin info for each render target
		and set the variant fields specifically*/
		m_render_targets[i].begin_info = render_pass_begin_info;
		m_render_targets[i].begin_info.framebuffer = m_render_targets[i].framebuffer;
		m_render_targets[i].begin_info.pClearValues = m_render_targets[i].clear_values.data();
	}
}

void MyScene::initGraphicsPipeline()
{
	auto vs = std::make_unique<Shader>("vs.spv");
	auto fs = std::make_unique<Shader>("fs.spv");
	
	VkPipelineShaderStageCreateInfo vs_stage{};
	vs_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vs_stage.pNext = NULL;
	vs_stage.flags = 0;
	vs_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vs_stage.module = vs->getModule();
	vs_stage.pName = "main";
	vs_stage.pSpecializationInfo = NULL;
	
	VkPipelineShaderStageCreateInfo fs_stage{};
	fs_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fs_stage.pNext = NULL;
	fs_stage.flags = 0;
	fs_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fs_stage.module = fs->getModule();
	fs_stage.pName = "main";
	fs_stage.pSpecializationInfo = NULL;
	
	VkPipelineShaderStageCreateInfo shader_stages[] = {vs_stage, fs_stage};
	
	VkVertexInputBindingDescription v_in_bind_desc[] =
	{
		{0, 12, VK_VERTEX_INPUT_RATE_VERTEX}
	};
	
	VkVertexInputAttributeDescription v_in_att_desc[] = 
	{
		{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}
	};
	
	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};
	vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state_create_info.pNext = NULL;
	vertex_input_state_create_info.flags = 0;
	vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
	vertex_input_state_create_info.pVertexBindingDescriptions = v_in_bind_desc;
	vertex_input_state_create_info.vertexAttributeDescriptionCount = 1;
	vertex_input_state_create_info.pVertexAttributeDescriptions = v_in_att_desc;
	
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state{};
	input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state.pNext = NULL;
	input_assembly_state.flags = 0;
	input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_state.primitiveRestartEnable = VK_FALSE;
	
	VkViewport viewport;
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = VulkanEngine::get().getSurfaceExtent().width;
	viewport.height = VulkanEngine::get().getSurfaceExtent().height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	
	VkRect2D scissors{VkOffset2D{0,0}, VulkanEngine::get().getSurfaceExtent()};
	
	VkPipelineViewportStateCreateInfo viewport_state{};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.pNext = NULL;
	viewport_state.flags = 0;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissors;
	
	VkPipelineRasterizationStateCreateInfo rast_state{};
	rast_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rast_state.pNext = NULL;
	rast_state.flags = 0;
	rast_state.depthClampEnable = VK_FALSE;
	rast_state.rasterizerDiscardEnable = VK_FALSE;
	rast_state.polygonMode = VK_POLYGON_MODE_FILL;
	rast_state.cullMode = VK_CULL_MODE_NONE;
	rast_state.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rast_state.depthBiasEnable = VK_FALSE;
	rast_state.lineWidth = 1.0f;
	
	VkPipelineMultisampleStateCreateInfo multi_state{};
	multi_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multi_state.pNext = NULL;
	multi_state.flags = 0;
	multi_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multi_state.sampleShadingEnable = VK_TRUE;
	multi_state.minSampleShading = 1.0f;
	multi_state.pSampleMask = NULL;
	multi_state.alphaToCoverageEnable = VK_FALSE;
	multi_state.alphaToOneEnable = VK_FALSE;
	
	VkPipelineColorBlendAttachmentState att_state{};
	att_state.blendEnable = VK_FALSE;
	att_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	
	VkPipelineColorBlendStateCreateInfo blend_state{};
	blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blend_state.pNext = NULL;
	blend_state.flags = 0;
	blend_state.logicOpEnable = VK_FALSE;
	blend_state.attachmentCount = 1;
	blend_state.pAttachments = &att_state;
	
	VkPipelineLayoutCreateInfo layout_info{};
	layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_info.pNext = NULL;
	layout_info.flags = 0;
	layout_info.setLayoutCount = 0;
	layout_info.pSetLayouts = NULL;
	layout_info.pushConstantRangeCount = 0;
	layout_info.pPushConstantRanges = NULL;
	
	VkPipelineLayout layout;
	vkCreatePipelineLayout(VulkanEngine::get().getDevice(), &layout_info, VK_NULL_HANDLE, &layout);
	
	VkGraphicsPipelineCreateInfo g_pipeline_create_info{};
	g_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	g_pipeline_create_info.pNext = NULL;
	g_pipeline_create_info.flags = 0;
	g_pipeline_create_info.stageCount = 2;
	g_pipeline_create_info.pStages = shader_stages;
	g_pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
	g_pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	g_pipeline_create_info.pTessellationState = NULL;
	g_pipeline_create_info.pViewportState = &viewport_state;
	g_pipeline_create_info.pRasterizationState = &rast_state;
	g_pipeline_create_info.pMultisampleState = &multi_state;
	g_pipeline_create_info.pDepthStencilState = NULL;
	g_pipeline_create_info.pColorBlendState = &blend_state;
	g_pipeline_create_info.pDynamicState = NULL;
	g_pipeline_create_info.layout = layout;
	g_pipeline_create_info.renderPass = m_render_pass;
	g_pipeline_create_info.subpass = 0;
	g_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	g_pipeline_create_info.basePipelineIndex = -1;
	
	vkCreateGraphicsPipelines(VulkanEngine::get().getDevice(), VK_NULL_HANDLE, 1, &g_pipeline_create_info, VK_NULL_HANDLE, &m_graphics_pipeline);
	
	vkDestroyPipelineLayout(VulkanEngine::get().getDevice(), layout, VK_NULL_HANDLE);
}

void MyScene::destroy()
{
	VkDevice d = VulkanEngine::get().getDevice();
	
	destroySynchronizationObjects();
	
	destroySurfaceDependentObjects();
	
	if (m_command_pool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(d, m_command_pool, VK_NULL_HANDLE);
		m_command_pool = VK_NULL_HANDLE;
	}
}

InputManager& MyScene::getInputManager()
{
    return *this;
}

Timer& MyScene::getTimer()
{
	return m_timer;
}

void MyScene::onResize()
{
	destroySurfaceDependentObjects();
	initSurfaceDependentObjects();
}

void MyScene::render()
{	
	VulkanEngine& e = VulkanEngine::get();
	uint32_t image_index;
	vkAcquireNextImageKHR(e.getDevice(), e.getSwapchain(), UINT64_MAX, m_semaphores[s_acquire_image], VK_NULL_HANDLE, &image_index);
	
	VkCommandBufferBeginInfo command_buffer_begin_info{};
	command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_buffer_begin_info.pNext = NULL;
	command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	command_buffer_begin_info.pInheritanceInfo = NULL;
	
	vkWaitForFences(e.getDevice(), 1, &m_fences[f_submit], VK_TRUE, UINT64_MAX);
	
	vkBeginCommandBuffer(m_command_buffers[image_index], &command_buffer_begin_info);
	
	//vkCmdBindPipeline(m_command_buffers[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics_pipeline);
	
		vkCmdBeginRenderPass(m_command_buffers[image_index], &m_render_targets[image_index].begin_info, VK_SUBPASS_CONTENTS_INLINE);
	
		vkCmdEndRenderPass(m_command_buffers[image_index]);
	
	vkEndCommandBuffer(m_command_buffers[image_index]);
	
	VkPipelineStageFlags submit_wait_flags[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	
	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &m_command_buffers[image_index];
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &m_semaphores[s_submit];
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &m_semaphores[s_acquire_image];
	submit_info.pWaitDstStageMask = submit_wait_flags;
	
	vkResetFences(e.getDevice(), 1, &m_fences[f_submit]);
	vkQueueSubmit(m_queue, 1, &submit_info, m_fences[f_submit]);
	
	VkPresentInfoKHR present_info{};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pImageIndices = &image_index;
	present_info.pNext = NULL;
	present_info.pResults = NULL;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &e.getSwapchain();
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &m_semaphores[s_submit];
	
	vkQueuePresentKHR(m_queue, &present_info);
}

void MyScene::KeyPressed(keycode_t k)
{
	switch(k)
	{
		case VKey_ESC:
			VulkanEngine::get().stop();
		break;
		default:
		break;
	}
}

void MyScene::KeyReleased(keycode_t)
{
	
}

void MyScene::MouseDragged(int16_t dx, int16_t dy)
{
	
}

void MyScene::MouseMoved(int16_t dx, int16_t dy)
{
	
}

void MyScene::MousePressed(mbflag_t)
{
	
}

void MyScene::MouseReleased(mbflag_t)
{
	
}

void MyScene::MouseScrolledDown()
{
	
}

void MyScene::MouseScrolledUp()
{
	
}
