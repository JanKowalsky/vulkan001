#ifndef MYSCENE_H
#define MYSCENE_H

#include "engine.h"
#include <vector>

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

	VkCommandPool m_command_pool = VK_NULL_HANDLE;
	VkCommandBuffer m_command_buffer = VK_NULL_HANDLE;
	VkRenderPass m_render_pass = VK_NULL_HANDLE;
	
	VkFence m_submit_fence = VK_NULL_HANDLE;
	
	VkSemaphore m_sem_acquire_image = VK_NULL_HANDLE;
	VkSemaphore m_sem_submit = VK_NULL_HANDLE;
	
	Timer m_timer;
};

#endif //MYSCENE_H