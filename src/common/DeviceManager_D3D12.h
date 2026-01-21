// DeviceManager_D3D12.h
// D3D12 implementation of device manager

#pragma once

#ifdef _WIN32

#include "DeviceManager.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <nvrhi/d3d12.h>
#include <nvrhi/validation.h>

namespace common
{
    class DeviceManager_D3D12 : public IDeviceManager
    {
    public:
        DeviceManager_D3D12();
        ~DeviceManager_D3D12() override;

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
        GraphicsAPI getGraphicsAPI() const override { return GraphicsAPI::D3D12; }
        const char* getGraphicsAPIName() const override { return "D3D12"; }

    private:
        bool createRenderTargets();
        void destroyRenderTargets();
        void waitForGPU();

    private:
        // Creation params
        DeviceCreationParams m_params;
        GLFWwindow* m_window = nullptr;
        HWND m_hwnd = nullptr;
        uint32_t m_windowWidth = 0;
        uint32_t m_windowHeight = 0;
        nvrhi::Format m_swapChainFormat = nvrhi::Format::RGBA8_UNORM;
        
        // D3D12 objects
        Microsoft::WRL::ComPtr<IDXGIFactory6> m_dxgiFactory;
        Microsoft::WRL::ComPtr<IDXGIAdapter1> m_adapter;
        Microsoft::WRL::ComPtr<ID3D12Device> m_d3d12Device;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
        Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapChain;
        Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
        HANDLE m_fenceEvent = nullptr;
        uint64_t m_fenceValue = 0;
        
        // NVRHI objects
        DefaultMessageCallback m_messageCallback;
        nvrhi::d3d12::DeviceHandle m_nvrhiDevice;
        nvrhi::DeviceHandle m_device;  // May be validation layer or direct device
        
        // Swap chain resources
        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_swapChainBuffers;
        std::vector<nvrhi::TextureHandle> m_swapChainTextures;
        std::vector<nvrhi::FramebufferHandle> m_framebuffers;
        uint32_t m_currentBackBuffer = 0;
    };

} // namespace common

#endif // _WIN32
