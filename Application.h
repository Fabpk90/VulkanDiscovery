#pragma once
#define GLFW_INCLUDE_VULKAN
#include <optional>
#include <vector>
#include <GLFW/glfw3.h>

class Application
{
public:
	//TODO: refactor this with the flag we want and XOR it
	//TODO: ex->  graphics = VK_GRAPHICS_BIT, and then props[i].queueflag ^ graphicsFamily
	struct FQueueFamily
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete()
		{
			return graphicsFamily.has_value()
			&& presentFamily.has_value();
		}
	};

	struct FSwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};
	
private:
	GLFWwindow* window;
	int32_t height;
	int32_t width;

	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkSurfaceKHR surface;
	
	std::vector<VkLayerProperties> availableLayers;

	//used to check if extensions are available (for now the swapchain, to present stuff on the screen)
	const std::vector<const char*> deviceExtensions = 
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	
public:
	Application(int32_t height, int32_t width, const char* windowName);
	Application(const Application& app) = delete;
	Application(const Application&& app) = delete;
	~Application();

	void run();

private:
	void createSurface();
	
	void pickPhysicalDevice();
	void pickLogicalDevice();

	bool checkValidationLayerSupport();
	
	bool canDeviceSupportExtensions(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	FQueueFamily queryQueueFamilies(VkPhysicalDevice device);
	FSwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
};

