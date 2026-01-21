// DeviceManager_VK.cpp
// Vulkan implementation of device manager

#include "DeviceManager_VK.h"
#include <nvrhi/common/resource.h>

// Define storage for Vulkan-Hpp dynamic dispatch loader
// This must be defined in exactly one translation unit
#include <vulkan/vulkan.hpp>
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

namespace common
{

// Debug callback for Vulkan validation layers
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    const char* severity = "";
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        severity = "[ERROR]";
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        severity = "[WARNING]";
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        severity = "[INFO]";
    else
        severity = "[VERBOSE]";
    
    std::cerr << "[Vulkan] " << severity << " " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

DeviceManager_VK::DeviceManager_VK() = default;

DeviceManager_VK::~DeviceManager_VK()
{
    destroyDevice();
}

void DeviceManager_VK::loadVulkanFunctions()
{
    // Get vkGetInstanceProcAddr from GLFW
    vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        glfwGetInstanceProcAddress(nullptr, "vkGetInstanceProcAddr"));
    
    if (!vkGetInstanceProcAddr)
    {
        std::cerr << "[Vulkan] Failed to get vkGetInstanceProcAddr" << std::endl;
        return;
    }
    
    // Initialize vulkan.hpp dispatch loader with vkGetInstanceProcAddr
    // This is required for NVRHI which uses vulkan.hpp internally
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
    
    // Load global functions
    vkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(
        vkGetInstanceProcAddr(nullptr, "vkCreateInstance"));
    vkEnumerateInstanceExtensionProperties = reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(
        vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceExtensionProperties"));
    vkEnumerateInstanceLayerProperties = reinterpret_cast<PFN_vkEnumerateInstanceLayerProperties>(
        vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceLayerProperties"));
}

void DeviceManager_VK::loadInstanceFunctions()
{
    if (!m_instance) return;
    
    // Initialize vulkan.hpp dispatch loader with instance (load instance-level functions)
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance, vkGetInstanceProcAddr, VK_NULL_HANDLE, nullptr);
    
    vkDestroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(
        vkGetInstanceProcAddr(m_instance, "vkDestroyInstance"));
    vkEnumeratePhysicalDevices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
        vkGetInstanceProcAddr(m_instance, "vkEnumeratePhysicalDevices"));
    vkGetPhysicalDeviceProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(
        vkGetInstanceProcAddr(m_instance, "vkGetPhysicalDeviceProperties"));
    vkGetPhysicalDeviceFeatures = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures>(
        vkGetInstanceProcAddr(m_instance, "vkGetPhysicalDeviceFeatures"));
    vkGetPhysicalDeviceQueueFamilyProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(
        vkGetInstanceProcAddr(m_instance, "vkGetPhysicalDeviceQueueFamilyProperties"));
    vkCreateDevice = reinterpret_cast<PFN_vkCreateDevice>(
        vkGetInstanceProcAddr(m_instance, "vkCreateDevice"));
    vkDestroySurfaceKHR = reinterpret_cast<PFN_vkDestroySurfaceKHR>(
        vkGetInstanceProcAddr(m_instance, "vkDestroySurfaceKHR"));
    vkGetPhysicalDeviceSurfaceSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(
        vkGetInstanceProcAddr(m_instance, "vkGetPhysicalDeviceSurfaceSupportKHR"));
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(
        vkGetInstanceProcAddr(m_instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
    vkGetPhysicalDeviceSurfaceFormatsKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(
        vkGetInstanceProcAddr(m_instance, "vkGetPhysicalDeviceSurfaceFormatsKHR"));
    vkGetPhysicalDeviceSurfacePresentModesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(
        vkGetInstanceProcAddr(m_instance, "vkGetPhysicalDeviceSurfacePresentModesKHR"));
    vkEnumerateDeviceExtensionProperties = reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(
        vkGetInstanceProcAddr(m_instance, "vkEnumerateDeviceExtensionProperties"));
    vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));
    vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
}

void DeviceManager_VK::loadDeviceFunctions()
{
    if (!m_vkDevice) return;
    
    // Get vkGetDeviceProcAddr for device-level function loading
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(
        vkGetInstanceProcAddr(m_instance, "vkGetDeviceProcAddr"));
    
    // Initialize vulkan.hpp dispatch loader with device (load device-level functions)
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance, vkGetInstanceProcAddr, m_vkDevice, vkGetDeviceProcAddr);
    
    auto getDeviceProc = [this](const char* name) {
        return vkGetInstanceProcAddr(m_instance, name);
    };
    
    vkDestroyDevice = reinterpret_cast<PFN_vkDestroyDevice>(getDeviceProc("vkDestroyDevice"));
    vkGetDeviceQueue = reinterpret_cast<PFN_vkGetDeviceQueue>(getDeviceProc("vkGetDeviceQueue"));
    vkCreateSwapchainKHR = reinterpret_cast<PFN_vkCreateSwapchainKHR>(getDeviceProc("vkCreateSwapchainKHR"));
    vkDestroySwapchainKHR = reinterpret_cast<PFN_vkDestroySwapchainKHR>(getDeviceProc("vkDestroySwapchainKHR"));
    vkGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(getDeviceProc("vkGetSwapchainImagesKHR"));
    vkAcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(getDeviceProc("vkAcquireNextImageKHR"));
    vkQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(getDeviceProc("vkQueuePresentKHR"));
    vkCreateSemaphore = reinterpret_cast<PFN_vkCreateSemaphore>(getDeviceProc("vkCreateSemaphore"));
    vkDestroySemaphore = reinterpret_cast<PFN_vkDestroySemaphore>(getDeviceProc("vkDestroySemaphore"));
    vkDeviceWaitIdle = reinterpret_cast<PFN_vkDeviceWaitIdle>(getDeviceProc("vkDeviceWaitIdle"));
    vkQueueWaitIdle = reinterpret_cast<PFN_vkQueueWaitIdle>(getDeviceProc("vkQueueWaitIdle"));
}

bool DeviceManager_VK::createDevice(const DeviceCreationParams& params)
{
    m_params = params;
    m_window = params.window;
    m_windowWidth = params.windowWidth;
    m_windowHeight = params.windowHeight;
    
    // Load Vulkan functions
    loadVulkanFunctions();
    if (!vkCreateInstance)
    {
        std::cerr << "[Vulkan] Failed to load Vulkan functions" << std::endl;
        return false;
    }
    
    if (!createInstance()) return false;
    loadInstanceFunctions();
    
    if (!createSurface()) return false;
    if (!selectPhysicalDevice()) return false;
    if (!findQueueFamilies()) return false;
    if (!createLogicalDevice()) return false;
    loadDeviceFunctions();
    
    // Create semaphores for synchronization
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    if (vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &m_imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore) != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to create semaphores" << std::endl;
        return false;
    }
    
    // Create NVRHI Vulkan device
    nvrhi::vulkan::DeviceDesc deviceDesc = {};
    deviceDesc.errorCB = &m_messageCallback;
    deviceDesc.instance = m_instance;
    deviceDesc.physicalDevice = m_physicalDevice;
    deviceDesc.device = m_vkDevice;
    deviceDesc.graphicsQueue = m_graphicsQueue;
    deviceDesc.graphicsQueueIndex = static_cast<int>(m_graphicsQueueFamily);
    
    m_nvrhiDevice = nvrhi::vulkan::createDevice(deviceDesc);
    if (!m_nvrhiDevice)
    {
        std::cerr << "[Vulkan] Failed to create NVRHI device" << std::endl;
        return false;
    }
    
    // Optionally wrap with validation layer
    if (params.enableValidationLayer)
    {
        m_device = nvrhi::validation::createValidationLayer(m_nvrhiDevice);
        std::cout << "[Vulkan] NVRHI validation layer enabled" << std::endl;
    }
    else
    {
        m_device = m_nvrhiDevice;
    }
    
    // Create swap chain
    if (!createSwapChain())
    {
        return false;
    }
    
    std::cout << "[Vulkan] Device created successfully" << std::endl;
    return true;
}

bool DeviceManager_VK::createInstance()
{
    // Get required extensions from GLFW
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    
    // Add debug utils extension if debug layer is enabled
    if (m_params.enableDebugLayer)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    
    // Check for validation layer support
    std::vector<const char*> validationLayers;
    if (m_params.enableDebugLayer)
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        
        const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
        bool found = false;
        for (const auto& layer : availableLayers)
        {
            if (strcmp(layer.layerName, validationLayerName) == 0)
            {
                found = true;
                break;
            }
        }
        
        if (found)
        {
            validationLayers.push_back(validationLayerName);
            std::cout << "[Vulkan] Validation layer enabled" << std::endl;
        }
        else
        {
            std::cerr << "[Vulkan] Validation layer not available" << std::endl;
        }
    }
    
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "NVRHI Demo";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "NVRHI";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_4;
    
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
    
    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to create Vulkan instance" << std::endl;
        return false;
    }
    
    // Create debug messenger if debug layer is enabled
    if (m_params.enableDebugLayer && !validationLayers.empty())
    {
        loadInstanceFunctions();
        
        if (vkCreateDebugUtilsMessengerEXT)
        {
            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
            debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                         VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                         VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugCreateInfo.pfnUserCallback = debugCallback;
            
            vkCreateDebugUtilsMessengerEXT(m_instance, &debugCreateInfo, nullptr, &m_debugMessenger);
        }
    }
    
    return true;
}

bool DeviceManager_VK::createSurface()
{
    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to create window surface" << std::endl;
        return false;
    }
    return true;
}

bool DeviceManager_VK::selectPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    
    if (deviceCount == 0)
    {
        std::cerr << "[Vulkan] No GPUs with Vulkan support found" << std::endl;
        return false;
    }
    
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
    
    // Select the first discrete GPU, or the first available GPU
    for (const auto& device : devices)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        
        // Check for required extensions
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());
        
        bool hasSwapchain = false;
        for (const auto& ext : extensions)
        {
            if (strcmp(ext.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
            {
                hasSwapchain = true;
                break;
            }
        }
        
        if (!hasSwapchain)
            continue;
        
        // Prefer discrete GPU
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || m_physicalDevice == VK_NULL_HANDLE)
        {
            m_physicalDevice = device;
            std::cout << "[Vulkan] Using GPU: " << properties.deviceName << std::endl;
            
            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                break;
        }
    }
    
    if (m_physicalDevice == VK_NULL_HANDLE)
    {
        std::cerr << "[Vulkan] No suitable GPU found" << std::endl;
        return false;
    }
    
    return true;
}

bool DeviceManager_VK::findQueueFamilies()
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);
    
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());
    
    bool foundGraphics = false;
    bool foundPresent = false;
    
    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            m_graphicsQueueFamily = i;
            foundGraphics = true;
        }
        
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, m_surface, &presentSupport);
        if (presentSupport)
        {
            m_presentQueueFamily = i;
            foundPresent = true;
        }
        
        if (foundGraphics && foundPresent)
            break;
    }
    
    if (!foundGraphics || !foundPresent)
    {
        std::cerr << "[Vulkan] Failed to find suitable queue families" << std::endl;
        return false;
    }
    
    return true;
}

bool DeviceManager_VK::createLogicalDevice()
{
    std::set<uint32_t> uniqueQueueFamilies = { m_graphicsQueueFamily, m_presentQueueFamily };
    
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;
    
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    
    // Enable Vulkan 1.2 features required by NVRHI
    VkPhysicalDeviceVulkan12Features vulkan12Features = {};
    vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan12Features.timelineSemaphore = VK_TRUE;
    vulkan12Features.bufferDeviceAddress = VK_TRUE;
    
    // Enable Vulkan 1.3 features required by NVRHI (dynamic rendering)
    VkPhysicalDeviceVulkan13Features vulkan13Features = {};
    vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    vulkan13Features.dynamicRendering = VK_TRUE;
    vulkan13Features.synchronization2 = VK_TRUE;
    vulkan13Features.pNext = &vulkan12Features;
    
    // Basic device features
    VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &vulkan13Features;
    
    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &deviceFeatures2;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = nullptr;  // Using pNext chain instead
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    
    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_vkDevice) != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to create logical device" << std::endl;
        return false;
    }
    
    loadDeviceFunctions();
    
    vkGetDeviceQueue(m_vkDevice, m_graphicsQueueFamily, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_vkDevice, m_presentQueueFamily, 0, &m_presentQueue);
    
    return true;
}

void DeviceManager_VK::destroyDevice()
{
    waitForIdle();
    
    destroySwapChain();
    
    m_device = nullptr;
    m_nvrhiDevice = nullptr;
    
    if (m_imageAvailableSemaphore != VK_NULL_HANDLE && vkDestroySemaphore)
    {
        vkDestroySemaphore(m_vkDevice, m_imageAvailableSemaphore, nullptr);
        m_imageAvailableSemaphore = VK_NULL_HANDLE;
    }
    
    if (m_renderFinishedSemaphore != VK_NULL_HANDLE && vkDestroySemaphore)
    {
        vkDestroySemaphore(m_vkDevice, m_renderFinishedSemaphore, nullptr);
        m_renderFinishedSemaphore = VK_NULL_HANDLE;
    }
    
    if (m_vkDevice != VK_NULL_HANDLE && vkDestroyDevice)
    {
        vkDestroyDevice(m_vkDevice, nullptr);
        m_vkDevice = VK_NULL_HANDLE;
    }
    
    if (m_surface != VK_NULL_HANDLE && vkDestroySurfaceKHR)
    {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }
    
    if (m_debugMessenger != VK_NULL_HANDLE && vkDestroyDebugUtilsMessengerEXT)
    {
        vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
        m_debugMessenger = VK_NULL_HANDLE;
    }
    
    if (m_instance != VK_NULL_HANDLE && vkDestroyInstance)
    {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }
}

bool DeviceManager_VK::createSwapChain()
{
    // Get surface capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &capabilities);
    
    // Get surface formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, formats.data());
    
    // Get present modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, presentModes.data());
    
    // Choose swap surface format
    VkSurfaceFormatKHR surfaceFormat = formats[0];
    for (const auto& format : formats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            surfaceFormat = format;
            break;
        }
    }
    m_swapChainFormat = nvrhi::Format::BGRA8_UNORM;
    
    // Choose present mode
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;  // Always available
    if (!m_params.vsync)
    {
        for (const auto& mode : presentModes)
        {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                presentMode = mode;
                break;
            }
            if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            {
                presentMode = mode;
            }
        }
    }
    
    // Choose extent
    VkExtent2D extent;
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        extent = capabilities.currentExtent;
    }
    else
    {
        extent.width = std::clamp(m_windowWidth, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(m_windowHeight, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }
    m_windowWidth = extent.width;
    m_windowHeight = extent.height;
    
    // Choose image count
    uint32_t imageCount = m_params.swapChainBufferCount;
    if (imageCount < capabilities.minImageCount)
        imageCount = capabilities.minImageCount;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
        imageCount = capabilities.maxImageCount;
    
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    
    uint32_t queueFamilyIndices[] = { m_graphicsQueueFamily, m_presentQueueFamily };
    if (m_graphicsQueueFamily != m_presentQueueFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    
    if (vkCreateSwapchainKHR(m_vkDevice, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to create swap chain" << std::endl;
        return false;
    }
    
    // Get swap chain images
    vkGetSwapchainImagesKHR(m_vkDevice, m_swapChain, &imageCount, nullptr);
    m_swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_vkDevice, m_swapChain, &imageCount, m_swapChainImages.data());
    
    return createRenderTargets();
}

void DeviceManager_VK::destroySwapChain()
{
    destroyRenderTargets();
    
    if (m_swapChain != VK_NULL_HANDLE && vkDestroySwapchainKHR)
    {
        vkDestroySwapchainKHR(m_vkDevice, m_swapChain, nullptr);
        m_swapChain = VK_NULL_HANDLE;
    }
    m_swapChainImages.clear();
}

bool DeviceManager_VK::resizeSwapChain(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return true;
    
    m_windowWidth = width;
    m_windowHeight = height;
    
    waitForIdle();
    destroySwapChain();
    
    return createSwapChain();
}

bool DeviceManager_VK::createRenderTargets()
{
    uint32_t imageCount = static_cast<uint32_t>(m_swapChainImages.size());
    m_swapChainTextures.resize(imageCount);
    m_framebuffers.resize(imageCount);
    
    for (uint32_t i = 0; i < imageCount; i++)
    {
        nvrhi::TextureDesc textureDesc = {};
        textureDesc.width = m_windowWidth;
        textureDesc.height = m_windowHeight;
        textureDesc.format = m_swapChainFormat;
        textureDesc.dimension = nvrhi::TextureDimension::Texture2D;
        textureDesc.isRenderTarget = true;
        textureDesc.initialState = nvrhi::ResourceStates::Present;
        textureDesc.keepInitialState = true;
        textureDesc.debugName = "SwapChainBuffer" + std::to_string(i);
        
        m_swapChainTextures[i] = m_nvrhiDevice->createHandleForNativeTexture(
            nvrhi::ObjectTypes::VK_Image,
            nvrhi::Object(m_swapChainImages[i]),
            textureDesc);
        
        if (!m_swapChainTextures[i])
        {
            std::cerr << "[Vulkan] Failed to create texture handle for swap chain buffer " << i << std::endl;
            return false;
        }
        
        nvrhi::FramebufferDesc fbDesc = {};
        fbDesc.addColorAttachment(m_swapChainTextures[i]);
        
        m_framebuffers[i] = m_device->createFramebuffer(fbDesc);
        if (!m_framebuffers[i])
        {
            std::cerr << "[Vulkan] Failed to create framebuffer " << i << std::endl;
            return false;
        }
    }
    
    return true;
}

void DeviceManager_VK::destroyRenderTargets()
{
    m_framebuffers.clear();
    m_swapChainTextures.clear();
}

void DeviceManager_VK::beginFrame()
{
    // Acquire next image
    VkResult result = vkAcquireNextImageKHR(m_vkDevice, m_swapChain, UINT64_MAX,
        m_imageAvailableSemaphore, VK_NULL_HANDLE, &m_currentBackBuffer);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // Handle resize
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);
        resizeSwapChain(width, height);
    }
}

void DeviceManager_VK::present()
{
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 0;  // We're waiting for command list execution
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapChain;
    presentInfo.pImageIndices = &m_currentBackBuffer;
    
    vkQueuePresentKHR(m_presentQueue, &presentInfo);
    vkQueueWaitIdle(m_presentQueue);
    
    runGarbageCollection();
}

void DeviceManager_VK::waitForIdle()
{
    if (m_device)
    {
        m_device->waitForIdle();
    }
    if (m_vkDevice && vkDeviceWaitIdle)
    {
        vkDeviceWaitIdle(m_vkDevice);
    }
}

void DeviceManager_VK::runGarbageCollection()
{
    if (m_device)
    {
        m_device->runGarbageCollection();
    }
}

nvrhi::IDevice* DeviceManager_VK::getDevice() const
{
    return m_device.Get();
}

nvrhi::IFramebuffer* DeviceManager_VK::getCurrentFramebuffer() const
{
    return m_framebuffers[m_currentBackBuffer].Get();
}

nvrhi::ITexture* DeviceManager_VK::getCurrentBackBuffer() const
{
    return m_swapChainTextures[m_currentBackBuffer].Get();
}

nvrhi::CommandListHandle DeviceManager_VK::createCommandList() const
{
    return m_device->createCommandList();
}

void DeviceManager_VK::executeCommandList(nvrhi::ICommandList* commandList)
{
    m_device->executeCommandLists(&commandList, 1);
}

uint32_t DeviceManager_VK::getCurrentBackBufferIndex() const
{
    return m_currentBackBuffer;
}

uint32_t DeviceManager_VK::getBackBufferCount() const
{
    return static_cast<uint32_t>(m_swapChainImages.size());
}

} // namespace common
