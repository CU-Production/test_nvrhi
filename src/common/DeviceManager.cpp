// DeviceManager.cpp
// Common device manager implementation and factory functions

#include "DeviceManager.h"

#ifdef _WIN32
#include "DeviceManager_D3D12.h"
#endif
#include "DeviceManager_VK.h"

#include <iostream>

namespace common
{

// Default message callback implementation
void DefaultMessageCallback::message(nvrhi::MessageSeverity severity, const char* messageText)
{
    const char* severityStr = "";
    switch (severity)
    {
    case nvrhi::MessageSeverity::Info:    severityStr = "[INFO]"; break;
    case nvrhi::MessageSeverity::Warning: severityStr = "[WARNING]"; break;
    case nvrhi::MessageSeverity::Error:   severityStr = "[ERROR]"; break;
    case nvrhi::MessageSeverity::Fatal:   severityStr = "[FATAL]"; break;
    }
    std::cout << "[NVRHI] " << severityStr << " " << messageText << std::endl;
}

// Factory function to create device manager
std::unique_ptr<IDeviceManager> createDeviceManager(GraphicsAPI api)
{
    switch (api)
    {
#ifdef _WIN32
    case GraphicsAPI::D3D12:
        return std::make_unique<DeviceManager_D3D12>();
#endif
    case GraphicsAPI::Vulkan:
        return std::make_unique<DeviceManager_VK>();
    default:
        std::cerr << "Unsupported graphics API: " << graphicsAPIToString(api) << std::endl;
        return nullptr;
    }
}

// Get available graphics APIs on this platform
std::vector<GraphicsAPI> getAvailableGraphicsAPIs()
{
    std::vector<GraphicsAPI> apis;
    
#ifdef _WIN32
    apis.push_back(GraphicsAPI::D3D12);
#endif
    apis.push_back(GraphicsAPI::Vulkan);
    
    return apis;
}

// Convert API enum to string
const char* graphicsAPIToString(GraphicsAPI api)
{
    switch (api)
    {
    case GraphicsAPI::D3D12:  return "D3D12";
    case GraphicsAPI::Vulkan: return "Vulkan";
    default:                  return "Unknown";
    }
}

} // namespace common
