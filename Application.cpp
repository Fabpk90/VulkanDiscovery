#include "Application.h"

#include <iostream>
#include <vector>



const std::vector<const char*> validationLayers =
{
	"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
	const bool enableValidationLayer = true;
#else
	const bool enableValidationLayer = false;
#endif

Application::Application(int32_t height, int32_t width, const char* windowName)
	: height(height), width(width)
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(width, height, windowName, nullptr, nullptr);

	uint32_t availableLayersCount;
	vkEnumerateInstanceLayerProperties(&availableLayersCount, nullptr);

	availableLayers.reserve(availableLayersCount);
	vkEnumerateInstanceLayerProperties(&availableLayersCount, availableLayers.data());
	

	VkApplicationInfo info{};

	info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	info.pApplicationName = windowName;
	info.pEngineName = "Super Engine";
	info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	info.applicationVersion= VK_MAKE_VERSION(1, 0, 0);
	info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &info;

	uint32_t extensionRequired = 0;
	const char** extensionName;

	extensionName = glfwGetRequiredInstanceExtensions(&extensionRequired);

	createInfo.enabledExtensionCount = extensionRequired;
	createInfo.ppEnabledExtensionNames = extensionName;

	if (enableValidationLayer && checkValidationLayerSupport())
	{
		createInfo.enabledLayerCount = validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
		createInfo.enabledLayerCount = 0;

	VkResult res = vkCreateInstance(&createInfo, nullptr, &instance);

	if(res != VK_SUCCESS)
	{
		//ay caramba
		std::cout << "Ay caramba" << std::endl;
	}
	
	uint32_t extensionAvailable;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionAvailable, nullptr);

	std::vector<VkExtensionProperties> props(extensionAvailable);

	vkEnumerateInstanceExtensionProperties(nullptr, &extensionAvailable, props.data());

	std::cout << "Available extensions" << std::endl;
	for(const VkExtensionProperties& prop : props)
	{
		std::cout << prop.extensionName << std::endl;
	}

	std::cout << "Needed extensions" << std::endl;

	for(uint32_t i = 0; i < extensionRequired; i++)
	{
		std::cout << extensionName[i] << std::endl;
	}

	pickPhysicalDevice();
}

Application::~Application()
{
	vkDestroyInstance(instance, nullptr);
	
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Application::run()
{
	while(!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}
	}
}

bool Application::checkValidationLayerSupport()
{
	for (const char* layerName : validationLayers) 
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

void Application::pickPhysicalDevice()
{
	uint32_t availableDevices;
	vkEnumeratePhysicalDevices(instance, &availableDevices, nullptr);

	if(availableDevices == 0)
	{
		std::cout << "Ayy no device available !" << std::endl;
	}

	std::vector<VkPhysicalDevice> devices(availableDevices);
	vkEnumeratePhysicalDevices(instance, &availableDevices, devices.data());

	physicalDevice = VK_NULL_HANDLE;
	
	for(const VkPhysicalDevice& device : devices)
	{
		if(canDeviceSupportExtensions(device))
		{
			physicalDevice = device;
		}
	}

	if(physicalDevice == VK_NULL_HANDLE)
	{
		std::cout << "No suitable devices found !" << std::endl;
	}
}

bool Application::canDeviceSupportExtensions(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties deviceProps;
	vkGetPhysicalDeviceProperties(device, &deviceProps);

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	
	return deviceFeatures.geometryShader && deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}
