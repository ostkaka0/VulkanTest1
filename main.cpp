#include <iostream>
#include <vector>
#include <string>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                               \
    {                                                                          \
        g_vkFP##entrypoint =                                                   \
            (PFN_vk##entrypoint)vkGetInstanceProcAddr(inst, "vk" #entrypoint); \
        if (g_vkFP##entrypoint == NULL) {                                      \
            error("vkGetInstanceProcAddr failed to find vk" #entrypoint        \
			      "\n vkGetInstanceProcAddr Failure");                         \
        }                                                                      \
    }

void error(std::string error) {
	std::cout << error << std::endl;
	std::cin.get();
	exit(1);
}

std::vector<const char*> g_validationLayers;
std::vector<const char*> g_extensions;
VkInstance g_vkInstance;
VkPhysicalDevice g_vkGPU;
VkPhysicalDeviceProperties g_vkGPUProperties;
std::vector<VkQueueFamilyProperties> g_vkQueueProperties;

GLFWwindow* g_window = nullptr;
int g_width = 800;
int g_height = 600;

PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR g_vkFPGetPhysicalDeviceSurfaceCapabilitiesKHR;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR g_vkFPGetPhysicalDeviceSurfaceFormatsKHR;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR g_vkFPGetPhysicalDeviceSurfacePresentModesKHR;
PFN_vkGetPhysicalDeviceSurfaceSupportKHR g_vkFPGetPhysicalDeviceSurfaceSupportKHR;
PFN_vkCreateSwapchainKHR g_vkFPCreateSwapchainKHR;
PFN_vkDestroySwapchainKHR g_vkFPDestroySwapchainKHR;
PFN_vkGetSwapchainImagesKHR g_vkFPGetSwapchainImagesKHR;
PFN_vkAcquireNextImageKHR g_vkFPAcquireNextImageKHR;;
PFN_vkQueuePresentKHR g_vkFPQueuePresentKHR;

VkSurfaceKHR g_vkSurface;
uint32_t g_vkGraphicsQueueNodeIndex;

int main(int argc, char** argv) {
	
	g_validationLayers = std::vector<const char*> {
		"VK_LAYER_LUNARG_mem_tracker",
		"VK_LAYER_GOOGLE_unique_objects",
	};

	// Init GLFW
	{
		// Handle GLFW errors
		glfwSetErrorCallback([](int error, const char* description) {
			std::cout << "GLFW error: " << error << " - " << description << std::endl;
		});
		
		// Initialize GLFW
		if (!glfwInit())
			error("Cannot initialize GLFW.");

		// Check Vulkan support
		if (!glfwVulkanSupported())
			error("Cannot find compatible Vulkan client driver.");
	}

	// Get Validation layers
	{
		uint32_t numInstanceLayers = 0;

		// Get numInstanceLayers
		if (vkEnumerateInstanceLayerProperties(&numInstanceLayers, nullptr))
			error("Vulkan: Could not enumerate instance layer properties.");

		if (numInstanceLayers > 0) {
			std::vector<VkLayerProperties> instanceLayers(numInstanceLayers);
			if (vkEnumerateInstanceLayerProperties(&numInstanceLayers, instanceLayers.data()))
				error("Vulkan: Could not enumerate instance layer properties.");

			// Print layers:
			std::cout << "Validation layers: " << std::endl;
			for (int i = 0; i < numInstanceLayers; ++i) {
				std::cout << "\t" << instanceLayers[i].layerName << std::endl;
				std::cout << "\t\t" << instanceLayers[i].description << std::endl;
				std::cout << std::endl;
			}
			std::cout << std::endl;
		}
		else
			std::cout << "No validation layers found!" << std::endl;

		// TODO: Check Layers
	}

	// Check instance extensions
	{
		int numRequiredExtensions;
		const char** requiredExtensions;

		// Get required extensions from GLFW
		{
			requiredExtensions = glfwGetRequiredInstanceExtensions((int*)&numRequiredExtensions);

			if (numRequiredExtensions > 0) {
				// Write to global g_extensions
				for (int i = 0; i < numRequiredExtensions; ++i)
					g_extensions.push_back(requiredExtensions[i]);

				// Print
				std::cout << "Required Instance Extensions(GLFW):" << std::endl;
				for (int i = 0; i < numRequiredExtensions; ++i) {
					std::cout << "\t" << requiredExtensions[i] << std::endl;
				}
				std::cout << std::endl;
			}
			// TODO: Check extensions
		}

		// Get Instance extensions
		{
			VkResult err;
			uint32_t numInstanceExtensions;
			err = vkEnumerateInstanceExtensionProperties(nullptr, &numInstanceExtensions, nullptr);

			if (numInstanceExtensions > 0) {
				std::vector<VkExtensionProperties> instanceExtensions(numInstanceExtensions);
				err = vkEnumerateInstanceExtensionProperties(NULL, &numInstanceExtensions, instanceExtensions.data());

				// Print
				std::cout << "Instance Extensions: " << std::endl;
				for (int i = 0; i < numInstanceExtensions; ++i) {
					std::cout << "\t" <<instanceExtensions[i].extensionName << std::endl;
					std::cout << "\t\t" << instanceExtensions[i].specVersion << std::endl;
					std::cout << std::endl;
				}
				std::cout << std::endl;
			}
			// TODO: Check instance extensions(with required instance extensions)
		}
	}

	// Create Vulkan Instance
	{
		VkApplicationInfo app;
		{
			app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			app.pNext = nullptr;
			app.pApplicationName = "Vulkan test 1";
			app.applicationVersion = 0;
			app.pEngineName = "Vulkan test 1";
			app.engineVersion = 0;
			app.apiVersion = VK_API_VERSION;
		}

		VkInstanceCreateInfo instanceInfo;
		{
			instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			instanceInfo.pNext = nullptr;
			instanceInfo.pApplicationInfo = &app;
			instanceInfo.enabledLayerCount = g_validationLayers.size();
			instanceInfo.ppEnabledLayerNames = g_validationLayers.data();
			instanceInfo.enabledExtensionCount = g_extensions.size();
			instanceInfo.ppEnabledExtensionNames = g_extensions.data();
		}

		// TODO: Aligned allocators
		VkAllocationCallbacks allocator;
		{
			allocator.pUserData = nullptr;
			allocator.pfnAllocation = [](void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)->void* {
				return malloc(size);
			};
			allocator.pfnFree = [](void* pUserData, void* pMemory) {
				free(pMemory);
			};
			allocator.pfnReallocation = [](void* pUserData, void *pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) {
				free(pOriginal);
				return malloc(size);
			};
			allocator.pfnInternalAllocation = nullptr;
			allocator.pfnInternalFree = nullptr;
			allocator.pfnReallocation = nullptr;
		}
		
		// Create vulkan instance
		VkResult vkError = vkCreateInstance(&instanceInfo, &allocator, &g_vkInstance);

		// Handle errors
		switch (vkError) {
		case VK_ERROR_INCOMPATIBLE_DRIVER:
			error("Drivers do not support vulkan. Drivers could be outdated.");
			break;
		case VK_ERROR_EXTENSION_NOT_PRESENT:
			error("Cannot find specified extension.");
			break;
		case VK_SUCCESS:
			// Succes! (prevent default from catching success as error)
			std::cout << "Vulkan instance created!" << std::endl;
			break;
		default:
			error("Could not create vulkan Instance. Drivers could be outdated.");
			break;
		}

	}

	// Look for GPU device
	{
		uint32_t numGPUs;
		VkResult vkError = vkEnumeratePhysicalDevices(g_vkInstance, &numGPUs, nullptr);
		
		if (numGPUs < 0)
			error("vkEnumeratePhysicalDevices could not find any GPU devices.");

		if (vkError)
			error("vkEnumeratePhysicalDevices could not enumerate GPU devices.");

		if (numGPUs > 0) {
			std::vector<VkPhysicalDevice> physicalDevices(numGPUs);
			if (vkEnumeratePhysicalDevices(g_vkInstance, &numGPUs, physicalDevices.data()))
				error("vkEnumeratePhysicalDevices could not enumerate GPU devices.");

			g_vkGPU = physicalDevices[0];

			std::cout << numGPUs << " GPUs found!" << std::endl;
		}
	}

	// Get queue properties
	{
		uint32_t numQueues;
		vkGetPhysicalDeviceProperties(g_vkGPU, &g_vkGPUProperties);

		vkGetPhysicalDeviceQueueFamilyProperties(g_vkGPU, &numQueues, nullptr);
		if (numQueues == 0)
			error("vkGetPhysicalDeviceQueueFamilyProperties could not find any queues.");

		g_vkQueueProperties = std::vector<VkQueueFamilyProperties>(numQueues);
		vkGetPhysicalDeviceQueueFamilyProperties(g_vkGPU, &numQueues, g_vkQueueProperties.data());
	}

	// Look for device layers (Unecessary code that does nothing)
	{
		uint32_t numDeviceLayers;

		if (vkEnumerateDeviceLayerProperties(g_vkGPU, &numDeviceLayers, nullptr))
			error("vkEnumerateDeviceLayerProperties failed!");

		if (numDeviceLayers > 0) {
			std::vector<VkLayerProperties> deviceLayers(numDeviceLayers);

			if (vkEnumerateDeviceLayerProperties(g_vkGPU, &numDeviceLayers, deviceLayers.data()))
				error("vkEnumerateDeviceLayerProperties failed!");

			// TODO: Check device layers.
		}
	}

	// Look for device extensions (swapchain extension)
	{
		uint32_t numDeviceExtensions;
		bool extensionSwapChainFound = false;

		if (vkEnumerateDeviceExtensionProperties(g_vkGPU, nullptr, &numDeviceExtensions, nullptr))
			error("vkEnumerateDeviceExtensionProperties failed!");

		if (numDeviceExtensions > 0) {
			std::vector<VkExtensionProperties> deviceExtensions(numDeviceExtensions);
			
			if (vkEnumerateDeviceExtensionProperties(g_vkGPU, nullptr, &numDeviceExtensions, deviceExtensions.data()))
				error("vkEnumerateDeviceExtensionProperties failed!");

			// Search for swapchain extension
			for (VkExtensionProperties extension : deviceExtensions) {
				if (!strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
					extensionSwapChainFound = true;
			}
			
			// Print
			std::cout << std::endl << "Extensions:" << std::endl;
			for (VkExtensionProperties extension : deviceExtensions) {
				std::cout << extension.extensionName << "(" << extension.specVersion << ")" << std::endl;
			}
			std::cout << std::endl;
		}

		if (!extensionSwapChainFound)
			error("Failed to find the " VK_KHR_SWAPCHAIN_EXTENSION_NAME " extension!");
	}

	// TODO: Validate

	// Get instance function adresses
	{
		GET_INSTANCE_PROC_ADDR(g_vkInstance, GetPhysicalDeviceSurfaceCapabilitiesKHR);
		GET_INSTANCE_PROC_ADDR(g_vkInstance, GetPhysicalDeviceSurfaceFormatsKHR);
		GET_INSTANCE_PROC_ADDR(g_vkInstance, GetPhysicalDeviceSurfacePresentModesKHR);
		GET_INSTANCE_PROC_ADDR(g_vkInstance, GetPhysicalDeviceSurfaceSupportKHR);
		GET_INSTANCE_PROC_ADDR(g_vkInstance, CreateSwapchainKHR);
		GET_INSTANCE_PROC_ADDR(g_vkInstance, DestroySwapchainKHR);
		GET_INSTANCE_PROC_ADDR(g_vkInstance, GetSwapchainImagesKHR);
		GET_INSTANCE_PROC_ADDR(g_vkInstance, AcquireNextImageKHR);
		GET_INSTANCE_PROC_ADDR(g_vkInstance, QueuePresentKHR);
	}

	// Create window
	{
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		g_window = glfwCreateWindow(g_width, g_height, "Vulkan test", NULL, NULL);

		if (!g_window)
			error("Could not create window!");

		glfwSetWindowRefreshCallback(g_window, [](GLFWwindow* window) {
			// TODO: draw();
		});
		glfwSetFramebufferSizeCallback(g_window, [](GLFWwindow* window, int width, int height) {
			g_width = width;
			g_height = height;
			// TODO: resize();
		});


	}

	// Init swapchain
	{
		glfwCreateWindowSurface(g_vkInstance, g_window, nullptr, &g_vkSurface);

		std::vector<VkBool32> supportsPresent(g_vkQueueProperties.size());

		for (uint32_t i = 0; i < g_vkQueueProperties.size(); ++i)
			g_vkFPGetPhysicalDeviceSurfaceSupportKHR(g_vkGPU, i, g_vkSurface, &supportsPresent[i]);

		uint32_t graphicsQueueNodeIndex = UINT32_MAX;
		uint32_t presentQueueNodeIndex = UINT32_MAX;

		for (uint32_t i = 0; i < g_vkQueueProperties.size(); ++i) {
			if (graphicsQueueNodeIndex == UINT32_MAX) {
				graphicsQueueNodeIndex = i;
			}

			if (supportsPresent[i] == VK_TRUE) {
				graphicsQueueNodeIndex = i;
				presentQueueNodeIndex = i;
				break;
			}
		}

		//if (presentQueueNodeIndex == UINT32_MAX) {
		//	for (uint32_t i = 0; i < g_vkQueueProperties.size(); ++i) {
		//		if (supportsPresent[i] == VK_TRUE) {
		//			presentQueueNodeIndex = i;
		//		}
		//	}
		//}

		if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX)
			error("Could not find a graphics and a present queue.");

		if (graphicsQueueNodeIndex != presentQueueNodeIndex)
			error("Could not find a common graphics and present queue.");

		g_vkGraphicsQueueNodeIndex = graphicsQueueNodeIndex;

		//TODO: init device
		{
			float queuePriotities = 0.f;
			VkDeviceQueueCreateInfo queue;
			{
				queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queue.pNext = NULL;
				queue.queueFamilyIndex = g_vkGraphicsQueueNodeIndex;
				queue.queueCount = 1;
				queue.pQueuePriorities = &queuePriotities;
			}

		}

		//vkGetDeviceQueue(g_vkDevice, g_vkGraphicsQueueNodeIndex, 0, g_vkQueue);

	}

	std::cin.get();

	return 0;
}
