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
	
public:
	Application(int32_t height, int32_t width, const char* windowName);
	Application(const Application& app) = delete;
	Application(const Application&& app) = delete;
	~Application();

	void run();

private:
	void createSurface();
	bool checkValidationLayerSupport();
	void pickPhysicalDevice();
	void pickLogicalDevice();
	bool canDeviceSupportExtensions(VkPhysicalDevice device);
	FQueueFamily findQueueFamilies(VkPhysicalDevice device);
};

