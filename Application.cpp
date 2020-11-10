#include "Application.h"


#include <fstream>
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
	createImageViews();
	createRenderPass();
	createGraphicPipeline();
	createFrameBuffer();
	createCommandPool();
	createCommandBuffers();
	createSemaphores();
}

Application::~Application()
{
	vkDestroySemaphore(logicalDevice, imageAvailableSemaphore, nullptr);
	vkDestroySemaphore(logicalDevice, renderFinishedSemaphore, nullptr);
	
	vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
	
	for(auto framebuffer : swapChainFramebuffers)
	{
		vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
	}
	
	vkDestroyPipeline(logicalDevice, pipeline, nullptr);
	vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
	vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);

	for(auto imageView : swapChainImageViews)
	{
		vkDestroyImageView(logicalDevice, imageView, nullptr);
	}
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
		drawFrame();

		if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}
	}
}

std::vector<char> Application::readFile(const char* fileName)
{
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);

	if(!file.is_open())
	{
		std::cout << "failed to open " << fileName << std::endl;
	}

	size_t fileSize = file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	return buffer;
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

void Application::createImageViews()
{
	swapChainImageViews.resize(swapChainImages.size());

	for(int i = 0; i < swapChainImages.size(); i++)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChainImageFormat;

		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.layerCount = 1;

		if(vkCreateImageView(logicalDevice, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
		{
			std::cout << "Could not create image views for image nb " << i << std::endl;
		}
	}
}

void Application::createRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

	//what do we do when we write into a "fresh" framebuffer ?
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	//not using a stencil buffer so don't care
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	//we don't care about the data remaining into the buffer, since we clear it
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	//but we care that after drawing to it, we are going to present it
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.colorAttachmentCount = 1;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	if(vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
	{
		std::cout << "Unable to create render pass" << std::endl;
	}
}

void Application::createGraphicPipeline()
{
	auto vertShaderCode = readFile("Shaders/vert.spv");
	auto fragShaderCode = readFile("Shaders/frag.spv");

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo createVertexStageInfo{};
	createVertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	createVertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	createVertexStageInfo.module = vertShaderModule;
	createVertexStageInfo.pName = "main";
	
	//this is useful, used to tell constant values, useful for letting the compiler optimize things
	//we don't use it here but cool to know
	createVertexStageInfo.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo createFragStageInfo{};
	createFragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	createFragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	createFragStageInfo.module = fragShaderModule;
	createFragStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { createVertexStageInfo, createFragStageInfo };

	//specifies the input of the vertex pipeline pass
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = swapChainExtent.width;
	viewport.height = swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.scissorCount = 1;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

	//we can bias the depth at a rasterizer level, cool ! (useful for some cool shadows ;) )
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	//there are other options, but since we're not doing any blending i'm leaving the default
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_B_BIT
	| VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_R_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	if(vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		std::cout << "Couldn't create pipeline layout" << std::endl;
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;

	pipelineInfo.layout = pipelineLayout;

	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;

	if(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline)
		!= VK_SUCCESS)
	{
		std::cout << "Unable to create the pipeline" << std::endl;
	}

	vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
}

void Application::createFrameBuffer()
{
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for(size_t i = 0; i < swapChainImageViews.size(); i++)
	{
		VkImageView attachments[] = {
			swapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if(vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
		{
			std::cout << "Unable to create framebuffer" << std::endl;
		}
	}
}

void Application::createCommandPool()
{
	FQueueFamily queueFamilyIndices = queryQueueFamilies(physicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	//this flag hints at how the pool should be treated
	//if for example we want to modify only a handful of commands or all
	poolInfo.flags = 0;

	if(vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
	{
		std::cout << "Unable to create the command pool" << std::endl;
	}
}

void Application::createCommandBuffers()
{
	commandBuffers.resize(swapChainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = commandBuffers.size();

	if(vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
	{
		std::cout << "Unable to create the command buffer" << std::endl;
	}

	for(size_t i = 0; i < commandBuffers.size(); i++)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		//beginInfo.flags -> usage of the buffer, useful for optimizing by re using stuff

		if(vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
		{
			std::cout << "Unable to record command buffer no " << i << std::endl;
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;

		VkClearValue clearColor = { 0, 0, 0, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
		vkCmdEndRenderPass(commandBuffers[i]);

		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
		{
			std::cout << "Unable to record the commands !" << std::endl;
		}
	}
}

void Application::createSemaphores()
{
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if(vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS
		|| vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS)
	{
		std::cout << "Unable to create the semaphores" << std::endl;
	}
}

void Application::drawFrame()
{
	uint32_t imageIndex;

	vkAcquireNextImageKHR(logicalDevice, swapchain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE
		, &imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphore[] = { imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		std::cout << "Unable to submit the queue" << std::endl;
	}

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0; // should always be > than src

	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;

	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapchain };
	presentInfo.pSwapchains = swapChains;
	presentInfo.swapchainCount = 1;
	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(presentQueue, &presentInfo);
	
}

VkShaderModule Application::createShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	
	VkShaderModule module;

	if(vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &module) != VK_SUCCESS)
	{
		std::cout << "Unable to create shader !" << std::endl;
	}

	return module;
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
