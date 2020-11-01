#include "Application.h"

#include <iostream>
#include <set>
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

	createSurface();

	pickPhysicalDevice();
	pickLogicalDevice();
}

Application::~Application()
{
	vkDestroyDevice(logicalDevice, nullptr);
	
	vkDestroySurfaceKHR(instance, surface, nullptr);
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

void Application::createSurface()
{
	if(glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
	{
		std::cout << "Ay caramba, the surface could not be created" << std::endl;
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

void Application::pickLogicalDevice()
{
	FQueueFamily families = findQueueFamilies(physicalDevice);

	
	float queuePriority = 1.0f;

	//we're creating the queues to send commands
	//in some cases the two queues can be in different devices that's why we use a set
	//if it's the same they will have te same val
	std::set<uint32_t> queueValues = { families.graphicsFamily.value(), families.presentFamily.value() };
	std::vector<VkDeviceQueueCreateInfo> queues(queueValues.size());
	
	for(uint32_t queueVal : queueValues)
	{
		VkDeviceQueueCreateInfo queueInfo{};

		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		queueInfo.queueFamilyIndex = queueVal;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &queuePriority;

		queues.emplace_back(queueInfo);
	}

	VkPhysicalDeviceFeatures features{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.pEnabledFeatures = &features;
	
	createInfo.pQueueCreateInfos = queues.data();
	createInfo.queueCreateInfoCount = queues.size();

	/*
	 * We should reference the validations layer previously set in the instance
	 * it's only required by the old implementation
	 */

	if(vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS)
	{
		std::cout << "Ay couldn't create the logical device" << std::endl;
	}
	else
	{
		vkGetDeviceQueue(logicalDevice, families.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(logicalDevice, families.presentFamily.value(), 0, &presentQueue);
	}

	
}

bool Application::canDeviceSupportExtensions(VkPhysicalDevice device)
{
	FQueueFamily familiy = findQueueFamilies(device);
	
	return familiy.isComplete();
}

Application::FQueueFamily Application::findQueueFamilies(VkPhysicalDevice device)
{
	FQueueFamily queueFamily{};

	uint32_t familyQueueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &familyQueueCount, nullptr);

	std::vector<VkQueueFamilyProperties> props(familyQueueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &familyQueueCount, props.data());

	for(int i = 0; i < props.size(); i++)
	{
		if(props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			queueFamily.graphicsFamily = i;

			if (queueFamily.isComplete())
				break;
		}
		else
		{
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (presentSupport)
			{
				queueFamily.presentFamily = i;

				if (queueFamily.isComplete())
					break;
			}
		}
	}

	return queueFamily;
}
