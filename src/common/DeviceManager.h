// DeviceManager.h
// Abstract interface for GPU device management supporting multiple backends (D3D12, Vulkan)

#pragma once

#include <nvrhi/nvrhi.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>

struct GLFWwindow;

namespace common
{
    // Supported graphics API backends
    enum class GraphicsAPI
    {
        D3D12,
        Vulkan
    };

    // Device creation parameters
    struct DeviceCreationParams
    {
        // Window handle
        GLFWwindow* window = nullptr;
        
        // Swap chain settings
        uint32_t swapChainBufferCount = 2;
        uint32_t windowWidth = 1280;
        uint32_t windowHeight = 720;
        nvrhi::Format swapChainFormat = nvrhi::Format::RGBA8_UNORM;
        bool vsync = true;
        
        // Debug settings
        bool enableDebugLayer = true;
        bool enableValidationLayer = true;
        
        // Device selection (use first suitable device if empty)
        std::string preferredAdapterName;
    };

    // Message callback for NVRHI errors and warnings
    class DefaultMessageCallback : public nvrhi::IMessageCallback
    {
    public:
        void message(nvrhi::MessageSeverity severity, const char* messageText) override;
    };

    // Abstract device manager interface
    class IDeviceManager
    {
    public:
        virtual ~IDeviceManager() = default;

        // Initialization and shutdown
        virtual bool createDevice(const DeviceCreationParams& params) = 0;
        virtual void destroyDevice() = 0;

        // Swap chain management
        virtual bool createSwapChain() = 0;
        virtual void destroySwapChain() = 0;
        virtual bool resizeSwapChain(uint32_t width, uint32_t height) = 0;
        
        // Frame management
        virtual void beginFrame() = 0;
        virtual void present() = 0;
        
        // Getters
        virtual nvrhi::IDevice* getDevice() const = 0;
        virtual nvrhi::IFramebuffer* getCurrentFramebuffer() const = 0;
        virtual nvrhi::ITexture* getCurrentBackBuffer() const = 0;
        virtual nvrhi::CommandListHandle createCommandList() const = 0;
        virtual void executeCommandList(nvrhi::ICommandList* commandList) = 0;
        virtual void waitForIdle() = 0;
        virtual void runGarbageCollection() = 0;
        
        virtual uint32_t getCurrentBackBufferIndex() const = 0;
        virtual uint32_t getBackBufferCount() const = 0;
        virtual uint32_t getWindowWidth() const = 0;
        virtual uint32_t getWindowHeight() const = 0;
        virtual nvrhi::Format getSwapChainFormat() const = 0;
        virtual GraphicsAPI getGraphicsAPI() const = 0;
        
        // Utility
        virtual const char* getGraphicsAPIName() const = 0;
    };

    // Factory function to create device manager for specified API
    std::unique_ptr<IDeviceManager> createDeviceManager(GraphicsAPI api);
    
    // Get available graphics APIs on this platform
    std::vector<GraphicsAPI> getAvailableGraphicsAPIs();
    
    // Convert API enum to string
    const char* graphicsAPIToString(GraphicsAPI api);

} // namespace common
