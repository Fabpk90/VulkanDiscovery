#include "Application.h"

#include <iostream>
#include <set>
#include <vector>



const std::vector<const char*> validationLayers =
{
	"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
	const bool enableValidationLayer = false;
#else
	const bool enableValidationLayer = true;
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

	if(vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
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

	createSwapChain();
}

Application::~Application()
{
	vkDestroySwapchainKHR(logicalDevice, swapchain, nullptr);
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

void Application::createSwapChain()
{
	FSwapChainSupportDetails swapChainDetails = querySwapChainSupport(physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainDetails.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainDetails.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainDetails.capabilities);

	// + 1 to make sure to not wait for the driver
	uint32_t imageCount = swapChainDetails.capabilities.minImageCount + 1;

	//making sure that with the +1 we don't ask for too many images
	// maxImageCount == 0 = no restrictions
	if(swapChainDetails.capabilities.maxImageCount > 0 && swapChainDetails.capabilities.maxImageCount < imageCount)
	{
		imageCount = swapChainDetails.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapInfo{};
	swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapInfo.surface = surface;
	swapInfo.minImageCount = imageCount;
	swapInfo.imageFormat = surfaceFormat.format;
	swapInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapInfo.imageExtent = extent;
	swapInfo.imageArrayLayers = 1;
	swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	FQueueFamily indices = queryQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	//if the two queue are not the same, we enter concurrent territory
	if(indices.graphicsFamily != indices.presentFamily)
	{
		swapInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapInfo.queueFamilyIndexCount = 2;
		swapInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapInfo.queueFamilyIndexCount = 0;
		swapInfo.pQueueFamilyIndices = nullptr;
	}

	//no transformations wanted
	swapInfo.preTransform = swapChainDetails.capabilities.currentTransform;

	//blending with other windows ? Nope
	swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	swapInfo.presentMode = presentMode;
	swapInfo.clipped = VK_TRUE;

	swapInfo.oldSwapchain = VK_NULL_HANDLE;

	if(vkCreateSwapchainKHR(logicalDevice, &swapInfo, nullptr, &swapchain) != VK_SUCCESS)
	{
		std::cout << "Could not create the swap chain" << std::endl;
	}

	vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, swapChainImages.data());

	swapChainExtent = extent;
	swapChainImageFormat = surfaceFormat.format;
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
	FQueueFamily families = queryQueueFamilies(physicalDevice);

	
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

	createInfo.enabledExtensionCount = deviceExtensions.size();
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

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

VkSurfaceFormatKHR Application::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for(const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB 
			&& availableFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
			return availableFormat;
	}

	return availableFormats[0];
}

VkPresentModeKHR Application::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentMode)
{
	for (const auto& presentMode : availablePresentMode)
	{
		//used for triple buffering
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return VK_PRESENT_MODE_MAILBOX_KHR;
	}

	//defaulting to the guaranteed to be available present mode
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Application::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	//we don't have a high dpi monitor
	if (capabilities.currentExtent.width != UINT32_MAX)
		return capabilities.currentExtent;
	else
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D extent{ width, height };

		extent.width = std::max(capabilities.minImageExtent.width,
			std::min(capabilities.maxImageExtent.width, extent.width));
		extent.height = std::max(capabilities.minImageExtent.height,
			std::min(capabilities.maxImageExtent.height, extent.height));

		return extent;
	}
}

bool Application::canDeviceSupportExtensions(VkPhysicalDevice device)
{
	FQueueFamily familiy = queryQueueFamilies(device);
	FSwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);

	
	return familiy.isComplete() && checkDeviceExtensionSupport(device)
	&& (!swapChainSupport.presentModes.empty() && !swapChainSupport.formats.empty());
}

bool Application::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;

	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for(const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}
	
	return requiredExtensions.empty();
}

Application::FQueueFamily Application::queryQueueFamilies(VkPhysicalDevice device)
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

Application::FSwapChainSupportDetails Application::querySwapChainSupport(VkPhysicalDevice device)
{
	FSwapChainSupportDetails details{};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	details.formats.resize(formatCount);
	
	if (formatCount != 0)
	{
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}
	else
		std::cout << "No format for the swap chain" << std::endl;

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	details.presentModes.resize(presentModeCount);

	if (presentModeCount != 0)
	{
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}
	else
		std::cout << "No present mode available for the swap chain " << std::endl;

	return details;
}
