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
	VkSwapchainKHR swapchain;
	
	std::vector<VkLayerProperties> availableLayers;

	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;

	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;

	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight; //used to wait for the image to be free to use
	size_t currentFrame = 0;
	

	//used to check if extensions are available (for now the swapchain, to present stuff on the screen)
	const std::vector<const char*> deviceExtensions = 
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

public:
	bool framebufferResized = false;
	
public:
	Application(int32_t height, int32_t width, const char* windowName);
	Application(const Application& app) = delete;
	Application(const Application&& app) = delete;
	~Application();

	void run();

	static std::vector<char> readFile(const char* fileName);

private:
	void cleanSwapChain();
	
	void createSurface();
	void createSwapChain();
	void createImageViews();
	void createRenderPass();
	void createGraphicsPipeline();
	void createFrameBuffer();
	void createCommandPool();
	void createCommandBuffers();
	void createSemaphores();
	void createFences();

	void recreateSwapChain();

	void drawFrame();
	
	VkShaderModule createShaderModule(const std::vector<char>& code);
	
	void pickPhysicalDevice();
	void pickLogicalDevice();

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentMode);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	bool checkValidationLayerSupport();
	
	bool canDeviceSupportExtensions(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	FQueueFamily queryQueueFamilies(VkPhysicalDevice device);
	FSwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
};

