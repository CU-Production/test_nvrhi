// DeviceManager_D3D12.cpp
// D3D12 implementation of device manager

#ifdef _WIN32

#include "DeviceManager_D3D12.h"
#include <nvrhi/common/resource.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <iostream>

namespace common
{

DeviceManager_D3D12::DeviceManager_D3D12() = default;

DeviceManager_D3D12::~DeviceManager_D3D12()
{
    destroyDevice();
}

bool DeviceManager_D3D12::createDevice(const DeviceCreationParams& params)
{
    m_params = params;
    m_window = params.window;
    m_windowWidth = params.windowWidth;
    m_windowHeight = params.windowHeight;
    m_swapChainFormat = params.swapChainFormat;
    
    // Get native window handle
    m_hwnd = glfwGetWin32Window(m_window);
    if (!m_hwnd)
    {
        std::cerr << "[D3D12] Failed to get Win32 window handle" << std::endl;
        return false;
    }
    
    UINT dxgiFactoryFlags = 0;
    
    // Enable debug layer if requested
    if (params.enableDebugLayer)
    {
        Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
            std::cout << "[D3D12] Debug layer enabled" << std::endl;
        }
    }
    
    // Create DXGI factory
    if (FAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_dxgiFactory))))
    {
        std::cerr << "[D3D12] Failed to create DXGI factory" << std::endl;
        return false;
    }
    
    // Find a suitable adapter
    for (UINT i = 0; m_dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
         IID_PPV_ARGS(&m_adapter)) != DXGI_ERROR_NOT_FOUND; i++)
    {
        DXGI_ADAPTER_DESC1 desc;
        m_adapter->GetDesc1(&desc);
        
        // Skip software adapters
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;
        
        // Check if adapter supports D3D12
        if (SUCCEEDED(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))
        {
            std::wcout << L"[D3D12] Using GPU: " << desc.Description << std::endl;
            break;
        }
    }
    
    if (!m_adapter)
    {
        std::cerr << "[D3D12] No suitable GPU adapter found" << std::endl;
        return false;
    }
    
    // Create D3D12 device
    if (FAILED(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_d3d12Device))))
    {
        std::cerr << "[D3D12] Failed to create D3D12 device" << std::endl;
        return false;
    }
    
    // Create command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    
    if (FAILED(m_d3d12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue))))
    {
        std::cerr << "[D3D12] Failed to create command queue" << std::endl;
        return false;
    }
    
    // Create fence for synchronization
    if (FAILED(m_d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence))))
    {
        std::cerr << "[D3D12] Failed to create fence" << std::endl;
        return false;
    }
    
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_fenceEvent)
    {
        std::cerr << "[D3D12] Failed to create fence event" << std::endl;
        return false;
    }
    
    // Create NVRHI D3D12 device
    nvrhi::d3d12::DeviceDesc deviceDesc = {};
    deviceDesc.errorCB = &m_messageCallback;
    deviceDesc.pDevice = m_d3d12Device.Get();
    deviceDesc.pGraphicsCommandQueue = m_commandQueue.Get();
    
    m_nvrhiDevice = nvrhi::d3d12::createDevice(deviceDesc);
    if (!m_nvrhiDevice)
    {
        std::cerr << "[D3D12] Failed to create NVRHI device" << std::endl;
        return false;
    }
    
    // Optionally wrap with validation layer
    if (params.enableValidationLayer)
    {
        m_device = nvrhi::validation::createValidationLayer(m_nvrhiDevice);
        std::cout << "[D3D12] Validation layer enabled" << std::endl;
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
    
    std::cout << "[D3D12] Device created successfully" << std::endl;
    return true;
}

void DeviceManager_D3D12::destroyDevice()
{
    waitForIdle();
    
    destroySwapChain();
    
    m_device = nullptr;
    m_nvrhiDevice = nullptr;
    
    if (m_fenceEvent)
    {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }
    m_fence.Reset();
    m_commandQueue.Reset();
    m_d3d12Device.Reset();
    m_adapter.Reset();
    m_dxgiFactory.Reset();
}

bool DeviceManager_D3D12::createSwapChain()
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = m_windowWidth;
    swapChainDesc.Height = m_windowHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = m_params.swapChainBufferCount;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    
    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
    if (FAILED(m_dxgiFactory->CreateSwapChainForHwnd(
        m_commandQueue.Get(), m_hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1)))
    {
        std::cerr << "[D3D12] Failed to create swap chain" << std::endl;
        return false;
    }
    
    // Disable Alt+Enter fullscreen toggle
    m_dxgiFactory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER);
    
    if (FAILED(swapChain1.As(&m_swapChain)))
    {
        std::cerr << "[D3D12] Failed to get IDXGISwapChain4 interface" << std::endl;
        return false;
    }
    
    return createRenderTargets();
}

void DeviceManager_D3D12::destroySwapChain()
{
    destroyRenderTargets();
    m_swapChain.Reset();
}

bool DeviceManager_D3D12::resizeSwapChain(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return true;
    
    m_windowWidth = width;
    m_windowHeight = height;
    
    waitForGPU();
    destroyRenderTargets();
    
    if (FAILED(m_swapChain->ResizeBuffers(
        m_params.swapChainBufferCount,
        width,
        height,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH)))
    {
        std::cerr << "[D3D12] Failed to resize swap chain" << std::endl;
        return false;
    }
    
    return createRenderTargets();
}

bool DeviceManager_D3D12::createRenderTargets()
{
    m_swapChainBuffers.resize(m_params.swapChainBufferCount);
    m_swapChainTextures.resize(m_params.swapChainBufferCount);
    m_framebuffers.resize(m_params.swapChainBufferCount);
    
    for (UINT i = 0; i < m_params.swapChainBufferCount; i++)
    {
        if (FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_swapChainBuffers[i]))))
        {
            std::cerr << "[D3D12] Failed to get swap chain buffer " << i << std::endl;
            return false;
        }
        
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
            nvrhi::ObjectTypes::D3D12_Resource,
            static_cast<nvrhi::Object>(m_swapChainBuffers[i].Get()),
            textureDesc);
        
        if (!m_swapChainTextures[i])
        {
            std::cerr << "[D3D12] Failed to create texture handle for swap chain buffer " << i << std::endl;
            return false;
        }
        
        nvrhi::FramebufferDesc fbDesc = {};
        fbDesc.addColorAttachment(m_swapChainTextures[i]);
        
        m_framebuffers[i] = m_device->createFramebuffer(fbDesc);
        if (!m_framebuffers[i])
        {
            std::cerr << "[D3D12] Failed to create framebuffer " << i << std::endl;
            return false;
        }
    }
    
    return true;
}

void DeviceManager_D3D12::destroyRenderTargets()
{
    m_framebuffers.clear();
    m_swapChainTextures.clear();
    m_swapChainBuffers.clear();
}

void DeviceManager_D3D12::beginFrame()
{
    m_currentBackBuffer = m_swapChain->GetCurrentBackBufferIndex();
}

void DeviceManager_D3D12::present()
{
    m_swapChain->Present(m_params.vsync ? 1 : 0, 0);
    waitForGPU();
    runGarbageCollection();
}

void DeviceManager_D3D12::waitForGPU()
{
    m_fenceValue++;
    m_commandQueue->Signal(m_fence.Get(), m_fenceValue);
    
    if (m_fence->GetCompletedValue() < m_fenceValue)
    {
        m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

void DeviceManager_D3D12::waitForIdle()
{
    if (m_device)
    {
        m_device->waitForIdle();
    }
}

void DeviceManager_D3D12::runGarbageCollection()
{
    if (m_device)
    {
        m_device->runGarbageCollection();
    }
}

nvrhi::IDevice* DeviceManager_D3D12::getDevice() const
{
    return m_device.Get();
}

nvrhi::IFramebuffer* DeviceManager_D3D12::getCurrentFramebuffer() const
{
    return m_framebuffers[m_currentBackBuffer].Get();
}

nvrhi::ITexture* DeviceManager_D3D12::getCurrentBackBuffer() const
{
    return m_swapChainTextures[m_currentBackBuffer].Get();
}

nvrhi::CommandListHandle DeviceManager_D3D12::createCommandList() const
{
    return m_device->createCommandList();
}

void DeviceManager_D3D12::executeCommandList(nvrhi::ICommandList* commandList)
{
    m_device->executeCommandLists(&commandList, 1);
}

uint32_t DeviceManager_D3D12::getCurrentBackBufferIndex() const
{
    return m_currentBackBuffer;
}

uint32_t DeviceManager_D3D12::getBackBufferCount() const
{
    return m_params.swapChainBufferCount;
}

} // namespace common

#endif // _WIN32
