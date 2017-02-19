#include "myscene.h"
#include "shader.h"
#include <iostream>
#include <cstdlib>

constexpr const uint32_t res_x = 1920;
constexpr const uint32_t res_y = 1080;

enum SemNames{s_acquire_image, s_submit, num_sems};
enum FenNames{f_submit, num_fences};

int32_t findMemoryTypeIndex(uint32_t memory_type_bits, bool host_visible)
{
	const VkPhysicalDeviceMemoryProperties& props = VulkanEngine::get().getPhyDevMemProps();
	
	for(size_t i = 0; i < props.memoryTypeCount; i++)
	{
		if((memory_type_bits & (1 << i)) && (!host_visible || (props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)))
			return i;
	}
	
	return -1;
}

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
	initVertexBuffer();
	initDescriptorSets();
	
	initSurfaceDependentObjects();
}

void MyScene::initSurfaceDependentObjects()
{
	float ratio = (float)VulkanEngine::get().getSurfaceExtent().width / (float)VulkanEngine::get().getSurfaceExtent().height;
	m_camera = std::make_unique<Camera>(ratio, 1.0f, 1000.0f, M_PI_2);
	
	initRenderTargets();
	initGraphicsPipeline();
}

void MyScene::destroySurfaceDependentObjects()
{
	VkDevice d = VulkanEngine::get().getDevice();
	
	if(m_pipeline_layout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(d, m_pipeline_layout, VK_NULL_HANDLE);
		m_pipeline_layout = VK_NULL_HANDLE;
	}
	
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
	/*---Creating render pass---*/
	
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
			{VkClearColorValue{0, 0, 0, 0}}
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
	
	/*For each render target...*/
	for(size_t i = 0; i < m_render_targets.size(); i++)
	{
		/*Specify the attachment for each framebuffer as a different swapchain image view*/
		frambuffer_create_info.pAttachments = &VulkanEngine::get().getSwapchainImageViews()[i];
		/*Create a framebuffer for given render target*/
		vkCreateFramebuffer(VulkanEngine::get().getDevice(), &frambuffer_create_info, VK_NULL_HANDLE, &m_render_targets[i].framebuffer);
		
		/*Set the same clear values (defined earlier) for each render target*/
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

void MyScene::initVertexBuffer()
{
	VulkanEngine& e = VulkanEngine::get();
	
	/*---Creating buffer---*/
	
	/*Specify usage as storage texel buffer to store formatted vertex data,
	that can be read and written inside shaders*/
	VkBufferCreateInfo vb_create_info{};
	vb_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vb_create_info.pNext = NULL;
	vb_create_info.flags = 0;
	vb_create_info.size = sizeof(Vertex) * res_x * res_y;
	vb_create_info.usage = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
	vb_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	vb_create_info.queueFamilyIndexCount = 1;
	uint32_t qfi = e.getQueueFamilyIndexGeneral();
	vb_create_info.pQueueFamilyIndices = &qfi;
	
	vkCreateBuffer(e.getDevice(), &vb_create_info, VK_NULL_HANDLE, &m_vertex_buffer);
	
	/*Get memory requirements for buffer*/
	VkMemoryRequirements vb_mem_req;
	vkGetBufferMemoryRequirements(e.getDevice(), m_vertex_buffer, &vb_mem_req);
	
	VkMemoryAllocateInfo vb_mem_alloc_info{};
	vb_mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	vb_mem_alloc_info.pNext = NULL;
	vb_mem_alloc_info.allocationSize = vb_mem_req.size;
	vb_mem_alloc_info.memoryTypeIndex = findMemoryTypeIndex(vb_mem_req.memoryTypeBits, true);
	
	/*Allocate memory for the buffer*/
	vkAllocateMemory(e.getDevice(), &vb_mem_alloc_info, VK_NULL_HANDLE, &m_vertex_buffer_memory);
	
	/*Map buffer memory*/
	Vertex* mem;
	vkMapMemory(e.getDevice(), m_vertex_buffer_memory, 0, VK_WHOLE_SIZE, 0, (void**)&mem);
	
	constexpr const uint32_t numVerts = res_x * res_y;
	constexpr const float x_bound = 500.0f;
	constexpr const float y_bound = 500.0f;
	constexpr const float z_bound = 500.0f;
	
	/*Generate random initial location for each vertex, within specified bounds*/
	for(size_t v = 0; v < numVerts; v++)
	{
		mem[v].pos.x = (2*float((float)std::rand() / (float)RAND_MAX) - 1.0f) * x_bound;
		mem[v].pos.y = (2*float((float)std::rand() / (float)RAND_MAX) - 1.0f) * y_bound;
		mem[v].pos.z = (2*float((float)std::rand() / (float)RAND_MAX) - 1.0f) * z_bound;
	}
	
	/*Unmap buffer memory*/
	vkUnmapMemory(e.getDevice(), m_vertex_buffer_memory);
	
	/*Bind memory to the buffer*/
	vkBindBufferMemory(e.getDevice(), m_vertex_buffer, m_vertex_buffer_memory, 0);
	
	/*---Create vertex buffer view---*/
	
	VkBufferViewCreateInfo vb_view_create_info{};
	vb_view_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
	vb_view_create_info.pNext = NULL;
	vb_view_create_info.flags = 0;
	vb_view_create_info.buffer = m_vertex_buffer;
	vb_view_create_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	vb_view_create_info.offset = 0;
	vb_view_create_info.range = VK_WHOLE_SIZE;
	
	vkCreateBufferView(e.getDevice(), &vb_view_create_info, VK_NULL_HANDLE, &m_vertex_buffer_view);
}

void MyScene::initDescriptorSets()
{
	VkDevice d = VulkanEngine::get().getDevice();
	
	/*---Create descriptor set layout---*/
	
	VkDescriptorSetLayoutBinding vb_binding{};
	vb_binding.binding = 0;
	vb_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
	vb_binding.descriptorCount = 1;
	vb_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	vb_binding.pImmutableSamplers = NULL;
	
	VkDescriptorSetLayoutCreateInfo desc_set_layout_create_info{};
	desc_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	desc_set_layout_create_info.pNext = NULL;
	desc_set_layout_create_info.flags = 0;
	desc_set_layout_create_info.bindingCount = 1;
	desc_set_layout_create_info.pBindings = &vb_binding;
	
	vkCreateDescriptorSetLayout(d, &desc_set_layout_create_info, VK_NULL_HANDLE, &m_descriptor_set_layout);
	
	/*---Create descriptor pool---*/
	
	VkDescriptorPoolSize pool_size{};
	pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
	pool_size.descriptorCount = 1;
	
	VkDescriptorPoolCreateInfo desc_pool_create_info{};
	desc_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	desc_pool_create_info.pNext = NULL;
	desc_pool_create_info.flags = 0;
	desc_pool_create_info.maxSets = 1;
	desc_pool_create_info.poolSizeCount = 1;
	desc_pool_create_info.pPoolSizes = &pool_size;
	
	vkCreateDescriptorPool(d, &desc_pool_create_info, VK_NULL_HANDLE, &m_descriptor_pool);
	
	VkDescriptorSetAllocateInfo desc_set_allocate_info{};
	desc_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	desc_set_allocate_info.pNext = NULL;
	desc_set_allocate_info.descriptorPool = m_descriptor_pool;
	desc_set_allocate_info.descriptorSetCount = 1;
	desc_set_allocate_info.pSetLayouts = &m_descriptor_set_layout;
	
	vkAllocateDescriptorSets(d, &desc_set_allocate_info, &m_descriptor_set);
	
	VkWriteDescriptorSet ds_write{};
	ds_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	ds_write.pNext = NULL;
	ds_write.dstSet = m_descriptor_set;
	ds_write.dstBinding = 0;
	ds_write.dstArrayElement = 0;
	ds_write.descriptorCount = 1;
	ds_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
	ds_write.pTexelBufferView = &m_vertex_buffer_view;
	
	vkUpdateDescriptorSets(d, 1, &ds_write, 0, NULL);
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
	
	std::vector<VkPipelineShaderStageCreateInfo> shader_stages = {vs_stage, fs_stage};
	
	/*std::vector<VkVertexInputBindingDescription> v_in_bind_desc =
	{
		{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX},
	};
	
	std::vector<VkVertexInputAttributeDescription> v_in_att_desc = 
	{
		{0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0},
		{1, 0, VK_FORMAT_R32G32B32_SFLOAT, 4*sizeof(float)}
	};*/
	
	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};
	vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state_create_info.pNext = NULL;
	vertex_input_state_create_info.flags = 0;
	vertex_input_state_create_info.vertexBindingDescriptionCount = 0;//v_in_bind_desc.size();
	vertex_input_state_create_info.pVertexBindingDescriptions = NULL;//v_in_bind_desc.data();
	vertex_input_state_create_info.vertexAttributeDescriptionCount = 0;//v_in_att_desc.size();
	vertex_input_state_create_info.pVertexAttributeDescriptions = NULL;//v_in_att_desc.data();
	
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
	layout_info.setLayoutCount = 1;
	layout_info.pSetLayouts = &m_descriptor_set_layout;
	layout_info.pushConstantRangeCount = 0;
	layout_info.pPushConstantRanges = NULL;
	
	vkCreatePipelineLayout(VulkanEngine::get().getDevice(), &layout_info, VK_NULL_HANDLE, &m_pipeline_layout);
	
	VkGraphicsPipelineCreateInfo g_pipeline_create_info{};
	g_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	g_pipeline_create_info.pNext = NULL;
	g_pipeline_create_info.flags = 0;
	g_pipeline_create_info.stageCount = shader_stages.size();
	g_pipeline_create_info.pStages = shader_stages.data();
	g_pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
	g_pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	g_pipeline_create_info.pTessellationState = NULL;
	g_pipeline_create_info.pViewportState = &viewport_state;
	g_pipeline_create_info.pRasterizationState = &rast_state;
	g_pipeline_create_info.pMultisampleState = &multi_state;
	g_pipeline_create_info.pDepthStencilState = NULL;
	g_pipeline_create_info.pColorBlendState = &blend_state;
	g_pipeline_create_info.pDynamicState = NULL;
	g_pipeline_create_info.layout = m_pipeline_layout;
	g_pipeline_create_info.renderPass = m_render_pass;
	g_pipeline_create_info.subpass = 0;
	g_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	g_pipeline_create_info.basePipelineIndex = -1;
	
	vkCreateGraphicsPipelines(VulkanEngine::get().getDevice(), VK_NULL_HANDLE, 1, &g_pipeline_create_info, VK_NULL_HANDLE, &m_graphics_pipeline);
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
	
	if(m_vertex_buffer_view != VK_NULL_HANDLE)
	{
		vkDestroyBufferView(d, m_vertex_buffer_view, VK_NULL_HANDLE);
		m_vertex_buffer_view = VK_NULL_HANDLE;
	}
	
	if(m_vertex_buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(d, m_vertex_buffer, VK_NULL_HANDLE);
		m_vertex_buffer = VK_NULL_HANDLE;
	}
	
	if(m_vertex_buffer_memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(d, m_vertex_buffer_memory, VK_NULL_HANDLE);
		m_vertex_buffer_memory = VK_NULL_HANDLE;
	}
	
	if(m_descriptor_set != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(d, m_descriptor_pool, VK_NULL_HANDLE);
		m_descriptor_pool = VK_NULL_HANDLE;
	}
	
	if(m_descriptor_set_layout != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(d, m_descriptor_set_layout, VK_NULL_HANDLE);
		m_descriptor_set_layout = VK_NULL_HANDLE;
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
	
	vkCmdBindPipeline(m_command_buffers[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics_pipeline);
	
	vkCmdBindDescriptorSets(m_command_buffers[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &m_descriptor_set, 0, NULL);
	
		vkCmdBeginRenderPass(m_command_buffers[image_index], &m_render_targets[image_index].begin_info, VK_SUBPASS_CONTENTS_INLINE);
	
		vkCmdDraw(m_command_buffers[image_index], res_x*res_y, 1, 0, 0);
	
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
