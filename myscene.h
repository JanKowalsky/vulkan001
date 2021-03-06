#ifndef MYSCENE_H
#define MYSCENE_H

#include <vector>
#include <memory>

#include "engine.h"
#include "vulkan_math.h"
#include "camera.h"
#include "recorder.h"

#include "myscene_utils.h"

class MyScene : public Scene, public InputManager
{

public:
	MyScene();
	~MyScene();

	virtual InputManager& getInputManager() override;
	virtual Timer& getTimer() override;
	virtual void onResize() override;
	void update();
	virtual void render() override;

	virtual void KeyPressed(keycode_t) override;
	virtual void KeyReleased(keycode_t) override;
	virtual void MouseDragged(int16_t dx, int16_t dy) override;
	virtual void MouseMoved(int16_t dx, int16_t dy) override;
	virtual void MousePressed(mbflag_t) override;
	virtual void MouseReleased(mbflag_t) override;
	virtual void MouseScrolledDown() override;
	virtual void MouseScrolledUp() override;

private:
	void initialize();
	void destroy();
	
	void initSurfaceDependentObjects();
	void destroySurfaceDependentObjects();

	void initSynchronizationObjects();
	void destroySynchronizationObjects();
	
	void destroyImage();
	
	void initCommandBuffers();
	void initImage();
	void initSampler();
	void initRenderTargets();
	void initGraphicsPipeline();
	void initRecordImages();
	void initVertexBuffer();
	void initDescriptorSets();
	
	void recordFrame(uint32_t);
	
	/*---Surface Independent---*/
	VkCommandPool m_command_pool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> m_command_buffers;
	
	std::vector<VkFence> m_fences;
	std::vector<VkSemaphore> m_semaphores;
	
	VkQueue m_queue = VK_NULL_HANDLE;
	
	std::vector<RecordImage> m_record_images;
	
	VkBuffer m_vertex_buffer = VK_NULL_HANDLE;
	VkBufferView m_vertex_buffer_view = VK_NULL_HANDLE;
	VkDeviceMemory m_vertex_buffer_memory = VK_NULL_HANDLE;
	
	VkSampler m_sampler = VK_NULL_HANDLE;
	VkDeviceMemory m_image_memory = VK_NULL_HANDLE;
	VkImage m_image = VK_NULL_HANDLE;
	VkImageView m_image_view = VK_NULL_HANDLE;
	
	VkDescriptorSetLayout m_descriptor_set_layout = VK_NULL_HANDLE;
	VkDescriptorPool m_descriptor_pool = VK_NULL_HANDLE;
	VkDescriptorSet m_descriptor_set = VK_NULL_HANDLE;
	
	/*---Surface Dependent---*/
	
	VkRenderPass m_render_pass = VK_NULL_HANDLE;
	std::vector<RenderTarget> m_render_targets;
	
	VkPipelineLayout m_pipeline_layout;
	VkPipeline m_graphics_pipeline = VK_NULL_HANDLE;
	
	std::unique_ptr<Camera> m_camera;
	
	/*---Other---*/
	Timer m_timer;
	VulkanRecorder m_recorder;
};

#endif //MYSCENE_H