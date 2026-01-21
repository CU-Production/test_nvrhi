// DeviceManager_VK.h
// Vulkan implementation of device manager

#pragma once

#include "DeviceManager.h"

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <nvrhi/vulkan.h>
#include <nvrhi/validation.h>

namespace common
{
    class DeviceManager_VK : public IDeviceManager
    {
    public:
        DeviceManager_VK();
        ~DeviceManager_VK() override;

        // IDeviceManager implementation
        bool createDevice(const DeviceCreationParams& params) override;
        void destroyDevice() override;
        
        bool createSwapChain() override;
        void destroySwapChain() override;
        bool resizeSwapChain(uint32_t width, uint32_t height) override;
        
        void beginFrame() override;
        void present() override;
        
        nvrhi::IDevice* getDevice() const override;
        nvrhi::IFramebuffer* getCurrentFramebuffer() const override;
        nvrhi::ITexture* getCurrentBackBuffer() const override;
        nvrhi::CommandListHandle createCommandList() const override;
        void executeCommandList(nvrhi::ICommandList* commandList) override;
        void waitForIdle() override;
        void runGarbageCollection() override;
        
        uint32_t getCurrentBackBufferIndex() const override;
        uint32_t getBackBufferCount() const override;
        uint32_t getWindowWidth() const override { return m_windowWidth; }
        uint32_t getWindowHeight() const override { return m_windowHeight; }
        nvrhi::Format getSwapChainFormat() const override { return m_swapChainFormat; }
        GraphicsAPI getGraphicsAPI() const override { return GraphicsAPI::Vulkan; }
        const char* getGraphicsAPIName() const override { return "Vulkan"; }

    private:
        bool createInstance();
        bool createSurface();
        bool selectPhysicalDevice();
        bool findQueueFamilies();
        bool createLogicalDevice();
        bool createRenderTargets();
        void destroyRenderTargets();
        
        // Vulkan function loading
        void loadVulkanFunctions();
        void loadInstanceFunctions();
        void loadDeviceFunctions();

    private:
        // Creation params
        DeviceCreationParams m_params;
        GLFWwindow* m_window = nullptr;
        uint32_t m_windowWidth = 0;
        uint32_t m_windowHeight = 0;
        nvrhi::Format m_swapChainFormat = nvrhi::Format::BGRA8_UNORM;
        
        // Vulkan objects
        VkInstance m_instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDevice m_vkDevice = VK_NULL_HANDLE;
        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        VkQueue m_presentQueue = VK_NULL_HANDLE;
        VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
        VkSemaphore m_imageAvailableSemaphore = VK_NULL_HANDLE;
        VkSemaphore m_renderFinishedSemaphore = VK_NULL_HANDLE;
        
        uint32_t m_graphicsQueueFamily = 0;
        uint32_t m_presentQueueFamily = 0;
        
        // NVRHI objects
        DefaultMessageCallback m_messageCallback;
        nvrhi::vulkan::DeviceHandle m_nvrhiDevice;
        nvrhi::DeviceHandle m_device;  // May be validation layer or direct device
        
        // Swap chain resources
        std::vector<VkImage> m_swapChainImages;
        std::vector<nvrhi::TextureHandle> m_swapChainTextures;
        std::vector<nvrhi::FramebufferHandle> m_framebuffers;
        uint32_t m_currentBackBuffer = 0;
        
        // Vulkan function pointers
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
        
        // Global functions
        PFN_vkCreateInstance vkCreateInstance = nullptr;
        PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties = nullptr;
        PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties = nullptr;
        
        // Instance functions
        PFN_vkDestroyInstance vkDestroyInstance = nullptr;
        PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices = nullptr;
        PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties = nullptr;
        PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures = nullptr;
        PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties = nullptr;
        PFN_vkCreateDevice vkCreateDevice = nullptr;
        PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR = nullptr;
        PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR = nullptr;
        PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
        PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR = nullptr;
        PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR = nullptr;
        PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties = nullptr;
        PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = nullptr;
        PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = nullptr;
        
        // Device functions
        PFN_vkDestroyDevice vkDestroyDevice = nullptr;
        PFN_vkGetDeviceQueue vkGetDeviceQueue = nullptr;
        PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR = nullptr;
        PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR = nullptr;
        PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR = nullptr;
        PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR = nullptr;
        PFN_vkQueuePresentKHR vkQueuePresentKHR = nullptr;
        PFN_vkCreateSemaphore vkCreateSemaphore = nullptr;
        PFN_vkDestroySemaphore vkDestroySemaphore = nullptr;
        PFN_vkDeviceWaitIdle vkDeviceWaitIdle = nullptr;
        PFN_vkQueueWaitIdle vkQueueWaitIdle = nullptr;
    };

} // namespace common
