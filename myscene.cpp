#include "myscene.h"
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
	VkCommandPoolCreateInfo command_pool_create_info{};
	command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_create_info.pNext = NULL;
	command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	command_pool_create_info.queueFamilyIndex = VulkanEngine::get().getQueueFamilyIndexGeneral();
	
	vkCreateCommandPool(VulkanEngine::get().getDevice(), &command_pool_create_info, VK_NULL_HANDLE, &m_command_pool);
	
	VkCommandBufferAllocateInfo command_buffer_allocate_info{};
	command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.pNext = NULL;
	command_buffer_allocate_info.commandPool = m_command_pool;
	command_buffer_allocate_info.commandBufferCount = 1;
	command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	
	vkAllocateCommandBuffers(VulkanEngine::get().getDevice(), &command_buffer_allocate_info, &m_command_buffer);
	
	VkFenceCreateInfo fence_create_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, NULL, VK_FENCE_CREATE_SIGNALED_BIT};
	m_fences.resize(num_fences);
	for(auto& f : m_fences)
	{
		vkCreateFence(VulkanEngine::get().getDevice(), &fence_create_info, VK_NULL_HANDLE, &f);
	}
	
	VkSemaphoreCreateInfo sem_create_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, NULL, 0};
	m_semaphores.resize(num_sems);
	for(auto& s : m_semaphores)
	{
		vkCreateSemaphore(VulkanEngine::get().getDevice(), &sem_create_info, VK_NULL_HANDLE, &s);
	}
	
}

void MyScene::destroy()
{
	for(auto& f : m_fences)
	{
		vkDestroyFence(VulkanEngine::get().getDevice(), f, VK_NULL_HANDLE);
		f = VK_NULL_HANDLE;
	}
	
	for(auto& s : m_semaphores)
	{
		if (s != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(VulkanEngine::get().getDevice(), s, VK_NULL_HANDLE);
			s = VK_NULL_HANDLE;
		}
	}
	
	if (m_command_pool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(VulkanEngine::get().getDevice(), m_command_pool, VK_NULL_HANDLE);
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
	
}

void MyScene::render()
{
	return;
	
	VulkanEngine& e = VulkanEngine::get();
	uint32_t image_index;
	vkAcquireNextImageKHR(e.getDevice(), e.getSwapchain(), UINT64_MAX, m_semaphores[s_acquire_image], VK_NULL_HANDLE, &image_index);
	
	VkCommandBufferBeginInfo command_buffer_begin_info{};
	command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_buffer_begin_info.pNext = NULL;
	command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	command_buffer_begin_info.pInheritanceInfo = NULL;
	
	vkWaitForFences(e.getDevice(), 1, &m_fences[f_submit], VK_TRUE, UINT64_MAX);
	vkResetFences(e.getDevice(), 1, &m_fences[f_submit]);
	
	vkBeginCommandBuffer(m_command_buffer, &command_buffer_begin_info);
	
	VkClearColorValue clear_color{1.0f, 0.0f, 0.0f, 1.0f};
	VkImageSubresourceRange sub_range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
	
	VkImageMemoryBarrier img_bar1{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		NULL,
		0,//VkAccessFlagBits::,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,
		e.getSwapchainImages()[image_index],
		sub_range
	};
	
	vkCmdClearColorImage(m_command_buffer,
	e.getSwapchainImages()[image_index],
	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	&clear_color,
	1,
	&sub_range);
	
	VkImageMemoryBarrier img_bar2{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		NULL,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		0,//VkAccessFlagBits::,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,
		e.getSwapchainImages()[image_index],
		sub_range
	};
	
	vkEndCommandBuffer(m_command_buffer);
	
	VkQueue queue;
	vkGetDeviceQueue(e.getDevice(), e.getQueueFamilyIndexGeneral(), 0, &queue);
	
	VkPipelineStageFlags submit_wait_flags[] = {VK_PIPELINE_STAGE_TRANSFER_BIT};
	
	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &m_command_buffer;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &m_semaphores[s_submit];
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &m_semaphores[s_acquire_image];
	submit_info.pWaitDstStageMask = submit_wait_flags;
	
	vkQueueSubmit(queue, 1, &submit_info, m_fences[f_submit]);
	
	VkPresentInfoKHR present_info{};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pImageIndices = &image_index;
	present_info.pNext = NULL;
	present_info.pResults = NULL;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &e.getSwapchain();
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &m_semaphores[s_acquire_image];
	
	vkQueuePresentKHR(queue, &present_info);
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
