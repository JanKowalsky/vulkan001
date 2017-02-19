#ifndef MYSCENE_H
#define MYSCENE_H

#include "engine.h"
#include <vector>

struct RenderTarget
{
	VkRenderPass render_pass;
	VkFramebuffer framebuffer;
	VkRenderPassBeginInfo begin_info;
	std::vector<VkClearValue> clear_values;
};

class MyScene : public Scene, public InputManager
{

public:
	MyScene();
	~MyScene();

	virtual InputManager& getInputManager() override;
	virtual Timer& getTimer() override;
	virtual void onResize() override;
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
	
	void initCommandBuffers();
	void initRenderTargets();
	void initGraphicsPipeline();

	/*---Surface Independent---*/
	VkCommandPool m_command_pool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> m_command_buffers;
	
	std::vector<VkFence> m_fences;
	std::vector<VkSemaphore> m_semaphores;
	
	VkQueue m_queue = VK_NULL_HANDLE;
	
	/*---Surface Dependent---*/
	VkRenderPass m_render_pass = VK_NULL_HANDLE;
	std::vector<RenderTarget> m_render_targets;
	VkPipeline m_graphics_pipeline = VK_NULL_HANDLE;
	
	/*---Other---*/
	Timer m_timer;
};

#endif //MYSCENE_H