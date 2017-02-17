#ifndef ENGINE_H
#define ENGINE_H

#include "Platform.h"
#include <vulkan.h>
#include <memory>
#include <vector>

#include "gui.h"
#include "Timer.h"
#include "Scene.h"

class VulkanEngine
{
public:
	static VulkanEngine& get();
	~VulkanEngine();

	void run();
	void stop();

	void onResize();
	
	void setScene(std::shared_ptr<Scene>);

	std::unique_ptr<VulkanWindow>& getWindow();
	InputManager& getInputManager();
	
	const VkDevice& getDevice() const noexcept;
	const VkSwapchainKHR& getSwapchain() const noexcept;
	const std::vector<VkImageView>& getSwapchainImageViews()const noexcept;
	const std::vector<VkImage>& getSwapchainImages() const noexcept;
	uint32_t getQueueFamilyIndexGeneral();
	uint32_t getQueueFamilyIndexTransfer();
	
private:
	VulkanEngine();
	void render();

	void EnableLayersAndExtensions();

	bool InitInstance();
	bool InitDevice();

	//SURFACE DEPENDENT-----------------------------------
	bool InitSurfaceDependentObjects();
	void DeinitSurfaceDependentObjects();
	bool InitSurface();
	bool InitSwapchain();

	void InitDebug();
	void DeInitDebug();
	
	bool Initialize();
	void Destroy();

	///////////////////////////////////////////////////////////////////////////////
	///////////////				SURFACE INDEPENDENT					///////////////
	///////////////////////////////////////////////////////////////////////////////

	//LAYERS AND EXTENSIONS--------------------------------------------------------
	std::vector<const char*> m_instance_layers;
	std::vector<const char*> m_instance_extensions;
	std::vector<const char*> m_device_extensions;

	VkDebugReportCallbackEXT debug_report = VK_NULL_HANDLE;
	
	//INSTANCE---------------------------------------------------------------------
	VkInstance m_instance = VK_NULL_HANDLE;

	//PHYSICAL DEVICE--------------------------------------------------------------
	VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;

	VkPhysicalDeviceProperties m_physical_device_properties;
	VkPhysicalDeviceFeatures m_physical_device_features;
	VkPhysicalDeviceMemoryProperties m_physical_device_memory_properties;

	//DEVICE-----------------------------------------------------------------------
	VkDevice m_device = VK_NULL_HANDLE;
	uint32_t m_queue_family_index_general = -1;
	uint32_t m_queue_family_index_transfer = -1;

	///////////////////////////////////////////////////////////////////////////////
	///////////////				SURFACE DEPENDENT					///////////////
	///////////////////////////////////////////////////////////////////////////////

	//SURFACE AND SWAPCHAIN--------------------------------------------------------
	VkSurfaceKHR m_surface = VK_NULL_HANDLE;

	uint32_t m_surface_size_x = 0;
	uint32_t m_surface_size_y = 0;
	VkFormat m_surface_format;

	VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
	std::vector<VkImage> m_swapchain_images;
	std::vector<VkImageView> m_swapchain_image_views;
	
	std::unique_ptr<VulkanWindow> m_window;
	std::shared_ptr<Scene> m_scene;
	
	bool m_running = true;
};

#endif //ENGINE_H