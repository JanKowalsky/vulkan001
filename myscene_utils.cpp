#include "myscene_utils.h"

#include "engine.h"

void RecordImage::destroy()
{
	VkDevice d = VulkanEngine::get().getDevice();
	
	if(img != VK_NULL_HANDLE)
	{
		vkDestroyImage(d, img, VK_NULL_HANDLE);
		img = VK_NULL_HANDLE;
	}
	
	if(img1 != VK_NULL_HANDLE)
	{
		vkDestroyImage(d, img1, VK_NULL_HANDLE);
		img1 = VK_NULL_HANDLE;
	}
	
	if(img_mem1 != VK_NULL_HANDLE)
	{
		vkFreeMemory(d, img_mem1, VK_NULL_HANDLE);
		img_mem1 = VK_NULL_HANDLE;
	}
	
	if(img_mem != VK_NULL_HANDLE)
	{
		vkFreeMemory(d, img_mem, VK_NULL_HANDLE);
		img_mem = VK_NULL_HANDLE;
	}
}

void VulkanImage::destroy()
{
	VkDevice d = VulkanEngine::get().getDevice();
	
	if(img != VK_NULL_HANDLE)
	{
		vkDestroyImage(d, img, VK_NULL_HANDLE);
		img = VK_NULL_HANDLE;
	}
	
	if(img_view != VK_NULL_HANDLE)
	{
		vkDestroyImageView(d, img_view, VK_NULL_HANDLE);
		img_view = VK_NULL_HANDLE;
	}
	
	if(mem != VK_NULL_HANDLE)
	{
		vkFreeMemory(d, mem, VK_NULL_HANDLE);
		mem = VK_NULL_HANDLE;
	}
}

void RenderTarget::destroy()
{
	if(framebuffer != VK_NULL_HANDLE)
	{
		vkDestroyFramebuffer(VulkanEngine::get().getDevice(), framebuffer, VK_NULL_HANDLE);
		framebuffer = VK_NULL_HANDLE;
	}
	target_image.destroy();
	depth_stencil_buffer.destroy();
}