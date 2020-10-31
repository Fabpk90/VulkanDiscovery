#pragma once
#define GLFW_INCLUDE_VULKAN
#include <vector>
#include <GLFW/glfw3.h>

class Application
{
private:
	GLFWwindow* window;
	int32_t height;
	int32_t width;

	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	
	std::vector<VkLayerProperties> availableLayers;
	
public:
	Application(int32_t height, int32_t width, const char* windowName);
	Application(const Application& app) = delete;
	Application(const Application&& app) = delete;
	~Application();

	void run();

private:
	bool checkValidationLayerSupport();
	void pickPhysicalDevice();
	bool canDeviceSupportExtensions(VkPhysicalDevice device);
};

