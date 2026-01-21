// NVRHI Triangle Demo
// This demo shows how to render a simple colored triangle using NVRHI with D3D12 backend

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <nvrhi/nvrhi.h>
#include <nvrhi/d3d12.h>
#include <nvrhi/validation.h>
#include <nvrhi/utils.h>
#include <nvrhi/common/resource.h>

#include <iostream>
#include <vector>
#include <array>
#include <fstream>
#include <filesystem>

using Microsoft::WRL::ComPtr;

// Window dimensions
constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;
constexpr int SWAP_CHAIN_BUFFER_COUNT = 2;

// Vertex structure matching shader input
struct Vertex
{
    float position[3];
    float color[3];
};

// Triangle vertices with position and color
static const std::array<Vertex, 3> g_TriangleVertices = {{
    // Position (x, y, z),       Color (r, g, b)
    {{  0.0f,   0.5f, 0.0f },  { 1.0f, 0.0f, 0.0f }},  // Top - Red
    {{  0.5f,  -0.5f, 0.0f },  { 0.0f, 1.0f, 0.0f }},  // Bottom Right - Green
    {{ -0.5f,  -0.5f, 0.0f },  { 0.0f, 0.0f, 1.0f }}   // Bottom Left - Blue
}};

// Message callback for NVRHI errors and warnings
class MessageCallback : public nvrhi::IMessageCallback
{
public:
    void message(nvrhi::MessageSeverity severity, const char* messageText) override
    {
        const char* severityStr = "";
        switch (severity)
        {
        case nvrhi::MessageSeverity::Info:    severityStr = "[INFO]"; break;
        case nvrhi::MessageSeverity::Warning: severityStr = "[WARNING]"; break;
        case nvrhi::MessageSeverity::Error:   severityStr = "[ERROR]"; break;
        case nvrhi::MessageSeverity::Fatal:   severityStr = "[FATAL]"; break;
        }
        std::cout << severityStr << " " << messageText << std::endl;
    }
};

// Application class encapsulating all rendering state
class TriangleApp
{
public:
    bool initialize();
    void mainLoop();
    void cleanup();

private:
    bool initWindow();
    bool initD3D12();
    bool initNVRHI();
    bool createSwapChain();
    bool createRenderTargets();
    bool loadShaders();
    bool createPipeline();
    bool createVertexBuffer();
    
    void render();
    void waitForGPU();
    void resizeSwapChain();
    
    std::vector<uint8_t> loadShaderFromFile(const std::string& filename);

private:
    // Window
    GLFWwindow* m_window = nullptr;
    HWND m_hwnd = nullptr;
    int m_windowWidth = WINDOW_WIDTH;
    int m_windowHeight = WINDOW_HEIGHT;
    bool m_windowResized = false;
    
    // D3D12 objects
    ComPtr<IDXGIFactory6> m_dxgiFactory;
    ComPtr<IDXGIAdapter1> m_adapter;
    ComPtr<ID3D12Device> m_d3d12Device;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<IDXGISwapChain4> m_swapChain;
    ComPtr<ID3D12Fence> m_fence;
    HANDLE m_fenceEvent = nullptr;
    uint64_t m_fenceValue = 0;
    
    // NVRHI objects
    MessageCallback m_messageCallback;
    nvrhi::d3d12::DeviceHandle m_nvrhiDevice;
    nvrhi::DeviceHandle m_validationLayer;
    nvrhi::CommandListHandle m_commandList;
    
    // Render targets
    std::vector<ComPtr<ID3D12Resource>> m_swapChainBuffers;
    std::vector<nvrhi::TextureHandle> m_swapChainTextures;
    std::vector<nvrhi::FramebufferHandle> m_framebuffers;
    uint32_t m_currentBackBuffer = 0;
    
    // Pipeline resources
    nvrhi::ShaderHandle m_vertexShader;
    nvrhi::ShaderHandle m_pixelShader;
    nvrhi::InputLayoutHandle m_inputLayout;
    nvrhi::GraphicsPipelineHandle m_pipeline;
    nvrhi::BufferHandle m_vertexBuffer;
};

bool TriangleApp::initialize()
{
    if (!initWindow()) return false;
    if (!initD3D12()) return false;
    if (!initNVRHI()) return false;
    if (!createSwapChain()) return false;
    if (!createRenderTargets()) return false;
    if (!loadShaders()) return false;
    if (!createPipeline()) return false;
    if (!createVertexBuffer()) return false;
    
    return true;
}

bool TriangleApp::initWindow()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    // No OpenGL context needed
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    
    m_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "NVRHI Triangle Demo", nullptr, nullptr);
    if (!m_window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    m_hwnd = glfwGetWin32Window(m_window);
    
    // Set user pointer for resize callback
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
        auto app = static_cast<TriangleApp*>(glfwGetWindowUserPointer(window));
        app->m_windowWidth = width;
        app->m_windowHeight = height;
        app->m_windowResized = true;
    });
    
    return true;
}

bool TriangleApp::initD3D12()
{
    UINT dxgiFactoryFlags = 0;
    
#ifdef _DEBUG
    // Enable debug layer
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif
    
    // Create DXGI factory
    if (FAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_dxgiFactory))))
    {
        std::cerr << "Failed to create DXGI factory" << std::endl;
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
            std::wcout << L"Using GPU: " << desc.Description << std::endl;
            break;
        }
    }
    
    if (!m_adapter)
    {
        std::cerr << "No suitable GPU adapter found" << std::endl;
        return false;
    }
    
    // Create D3D12 device
    if (FAILED(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_d3d12Device))))
    {
        std::cerr << "Failed to create D3D12 device" << std::endl;
        return false;
    }
    
    // Create command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    
    if (FAILED(m_d3d12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue))))
    {
        std::cerr << "Failed to create command queue" << std::endl;
        return false;
    }
    
    // Create fence for synchronization
    if (FAILED(m_d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence))))
    {
        std::cerr << "Failed to create fence" << std::endl;
        return false;
    }
    
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_fenceEvent)
    {
        std::cerr << "Failed to create fence event" << std::endl;
        return false;
    }
    
    return true;
}

bool TriangleApp::initNVRHI()
{
    // Create NVRHI D3D12 device
    nvrhi::d3d12::DeviceDesc deviceDesc = {};
    deviceDesc.errorCB = &m_messageCallback;
    deviceDesc.pDevice = m_d3d12Device.Get();
    deviceDesc.pGraphicsCommandQueue = m_commandQueue.Get();
    
    m_nvrhiDevice = nvrhi::d3d12::createDevice(deviceDesc);
    if (!m_nvrhiDevice)
    {
        std::cerr << "Failed to create NVRHI device" << std::endl;
        return false;
    }
    
    // Wrap with validation layer for debugging
    m_validationLayer = nvrhi::validation::createValidationLayer(m_nvrhiDevice);
    
    // Create command list
    m_commandList = m_validationLayer->createCommandList();
    if (!m_commandList)
    {
        std::cerr << "Failed to create command list" << std::endl;
        return false;
    }
    
    return true;
}

bool TriangleApp::createSwapChain()
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = m_windowWidth;
    swapChainDesc.Height = m_windowHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    
    ComPtr<IDXGISwapChain1> swapChain1;
    if (FAILED(m_dxgiFactory->CreateSwapChainForHwnd(
        m_commandQueue.Get(), m_hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1)))
    {
        std::cerr << "Failed to create swap chain" << std::endl;
        return false;
    }
    
    // Disable Alt+Enter fullscreen toggle
    m_dxgiFactory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER);
    
    if (FAILED(swapChain1.As(&m_swapChain)))
    {
        std::cerr << "Failed to get IDXGISwapChain4 interface" << std::endl;
        return false;
    }
    
    return true;
}

bool TriangleApp::createRenderTargets()
{
    m_swapChainBuffers.resize(SWAP_CHAIN_BUFFER_COUNT);
    m_swapChainTextures.resize(SWAP_CHAIN_BUFFER_COUNT);
    m_framebuffers.resize(SWAP_CHAIN_BUFFER_COUNT);
    
    for (UINT i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
    {
        if (FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_swapChainBuffers[i]))))
        {
            std::cerr << "Failed to get swap chain buffer " << i << std::endl;
            return false;
        }
        
        // Create NVRHI texture handle for the swap chain buffer
        nvrhi::TextureDesc textureDesc = {};
        textureDesc.width = m_windowWidth;
        textureDesc.height = m_windowHeight;
        textureDesc.format = nvrhi::Format::RGBA8_UNORM;
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
            std::cerr << "Failed to create texture handle for swap chain buffer " << i << std::endl;
            return false;
        }
        
        // Create framebuffer
        nvrhi::FramebufferDesc fbDesc = {};
        fbDesc.addColorAttachment(m_swapChainTextures[i]);
        
        m_framebuffers[i] = m_validationLayer->createFramebuffer(fbDesc);
        if (!m_framebuffers[i])
        {
            std::cerr << "Failed to create framebuffer " << i << std::endl;
            return false;
        }
    }
    
    return true;
}

std::vector<uint8_t> TriangleApp::loadShaderFromFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        return {};
    }
    
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    
    return buffer;
}

bool TriangleApp::loadShaders()
{
    // Try to load pre-compiled shaders first
    std::vector<uint8_t> vsData = loadShaderFromFile("shaders/triangle_vs.dxil");
    std::vector<uint8_t> psData = loadShaderFromFile("shaders/triangle_ps.dxil");
    
    if (vsData.empty() || psData.empty())
    {
        std::cerr << "Failed to load shader files. Please compile shaders first." << std::endl;
        std::cerr << "Run: slangc triangle.slang -profile sm_6_0 -target dxil -entry vsMain -stage vertex -o triangle_vs.dxil" << std::endl;
        std::cerr << "     slangc triangle.slang -profile sm_6_0 -target dxil -entry psMain -stage fragment -o triangle_ps.dxil" << std::endl;
        return false;
    }
    
    // Create vertex shader
    nvrhi::ShaderDesc vsDesc = {};
    vsDesc.shaderType = nvrhi::ShaderType::Vertex;
    vsDesc.debugName = "TriangleVS";
    vsDesc.entryName = "vsMain";
    
    m_vertexShader = m_validationLayer->createShader(vsDesc, vsData.data(), vsData.size());
    if (!m_vertexShader)
    {
        std::cerr << "Failed to create vertex shader" << std::endl;
        return false;
    }
    
    // Create pixel shader
    nvrhi::ShaderDesc psDesc = {};
    psDesc.shaderType = nvrhi::ShaderType::Pixel;
    psDesc.debugName = "TrianglePS";
    psDesc.entryName = "psMain";
    
    m_pixelShader = m_validationLayer->createShader(psDesc, psData.data(), psData.size());
    if (!m_pixelShader)
    {
        std::cerr << "Failed to create pixel shader" << std::endl;
        return false;
    }
    
    return true;
}

bool TriangleApp::createPipeline()
{
    // Define input layout matching vertex structure
    std::array<nvrhi::VertexAttributeDesc, 2> vertexAttributes = {{
        nvrhi::VertexAttributeDesc()
            .setName("POSITION")
            .setFormat(nvrhi::Format::RGB32_FLOAT)
            .setOffset(offsetof(Vertex, position))
            .setElementStride(sizeof(Vertex)),
        nvrhi::VertexAttributeDesc()
            .setName("COLOR")
            .setFormat(nvrhi::Format::RGB32_FLOAT)
            .setOffset(offsetof(Vertex, color))
            .setElementStride(sizeof(Vertex))
    }};
    
    m_inputLayout = m_validationLayer->createInputLayout(
        vertexAttributes.data(), 
        static_cast<uint32_t>(vertexAttributes.size()), 
        m_vertexShader);
    
    if (!m_inputLayout)
    {
        std::cerr << "Failed to create input layout" << std::endl;
        return false;
    }
    
    // Create graphics pipeline
    nvrhi::GraphicsPipelineDesc pipelineDesc = {};
    pipelineDesc.inputLayout = m_inputLayout;
    pipelineDesc.VS = m_vertexShader;
    pipelineDesc.PS = m_pixelShader;
    pipelineDesc.primType = nvrhi::PrimitiveType::TriangleList;
    
    // Configure render state
    pipelineDesc.renderState.depthStencilState.depthTestEnable = false;
    pipelineDesc.renderState.depthStencilState.depthWriteEnable = false;
    pipelineDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
    
    // Use framebuffer info for pipeline creation
    nvrhi::FramebufferInfo fbInfo;
    fbInfo.addColorFormat(nvrhi::Format::RGBA8_UNORM);
    
    m_pipeline = m_validationLayer->createGraphicsPipeline(pipelineDesc, fbInfo);
    if (!m_pipeline)
    {
        std::cerr << "Failed to create graphics pipeline" << std::endl;
        return false;
    }
    
    return true;
}

bool TriangleApp::createVertexBuffer()
{
    // Create vertex buffer
    nvrhi::BufferDesc bufferDesc = {};
    bufferDesc.byteSize = sizeof(Vertex) * g_TriangleVertices.size();
    bufferDesc.isVertexBuffer = true;
    bufferDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
    bufferDesc.keepInitialState = true;
    bufferDesc.debugName = "TriangleVertexBuffer";
    
    m_vertexBuffer = m_validationLayer->createBuffer(bufferDesc);
    if (!m_vertexBuffer)
    {
        std::cerr << "Failed to create vertex buffer" << std::endl;
        return false;
    }
    
    // Upload vertex data
    m_commandList->open();
    m_commandList->writeBuffer(m_vertexBuffer, g_TriangleVertices.data(), bufferDesc.byteSize);
    m_commandList->close();
    
    m_validationLayer->executeCommandLists(&m_commandList, 1);
    m_validationLayer->waitForIdle();
    
    return true;
}

void TriangleApp::render()
{
    // Handle window resize
    if (m_windowResized)
    {
        resizeSwapChain();
        m_windowResized = false;
    }
    
    // Skip rendering if window is minimized
    if (m_windowWidth == 0 || m_windowHeight == 0)
        return;
    
    // Get current back buffer index
    m_currentBackBuffer = m_swapChain->GetCurrentBackBufferIndex();
    
    // Begin recording commands
    m_commandList->open();
    
    // Clear render target to dark blue
    nvrhi::utils::ClearColorAttachment(m_commandList, m_framebuffers[m_currentBackBuffer], 0, 
        nvrhi::Color(0.1f, 0.1f, 0.2f, 1.0f));
    
    // Set up graphics state
    nvrhi::GraphicsState state = {};
    state.pipeline = m_pipeline;
    state.framebuffer = m_framebuffers[m_currentBackBuffer];
    state.viewport.addViewportAndScissorRect(nvrhi::Viewport(
        static_cast<float>(m_windowWidth), 
        static_cast<float>(m_windowHeight)));
    state.addVertexBuffer(nvrhi::VertexBufferBinding()
        .setBuffer(m_vertexBuffer)
        .setSlot(0)
        .setOffset(0));
    
    m_commandList->setGraphicsState(state);
    
    // Draw triangle
    nvrhi::DrawArguments drawArgs = {};
    drawArgs.vertexCount = static_cast<uint32_t>(g_TriangleVertices.size());
    m_commandList->draw(drawArgs);
    
    // End recording
    m_commandList->close();
    
    // Execute command list
    m_validationLayer->executeCommandLists(&m_commandList, 1);
    
    // Present
    m_swapChain->Present(1, 0);
    
    // Wait for GPU to finish
    waitForGPU();
    
    // Run garbage collection to release unused resources
    m_validationLayer->runGarbageCollection();
}

void TriangleApp::waitForGPU()
{
    m_fenceValue++;
    m_commandQueue->Signal(m_fence.Get(), m_fenceValue);
    
    if (m_fence->GetCompletedValue() < m_fenceValue)
    {
        m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

void TriangleApp::resizeSwapChain()
{
    if (m_windowWidth == 0 || m_windowHeight == 0)
        return;
    
    // Wait for GPU to finish before resizing
    waitForGPU();
    
    // Release old resources
    m_framebuffers.clear();
    m_swapChainTextures.clear();
    m_swapChainBuffers.clear();
    
    // Resize swap chain
    if (FAILED(m_swapChain->ResizeBuffers(
        SWAP_CHAIN_BUFFER_COUNT, 
        m_windowWidth, 
        m_windowHeight, 
        DXGI_FORMAT_R8G8B8A8_UNORM, 
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH)))
    {
        std::cerr << "Failed to resize swap chain" << std::endl;
        return;
    }
    
    // Recreate render targets
    createRenderTargets();
}

void TriangleApp::mainLoop()
{
    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();
        render();
    }
    
    // Wait for GPU before cleanup
    waitForGPU();
}

void TriangleApp::cleanup()
{
    // Release NVRHI resources
    m_vertexBuffer = nullptr;
    m_pipeline = nullptr;
    m_inputLayout = nullptr;
    m_pixelShader = nullptr;
    m_vertexShader = nullptr;
    m_commandList = nullptr;
    m_framebuffers.clear();
    m_swapChainTextures.clear();
    m_validationLayer = nullptr;
    m_nvrhiDevice = nullptr;
    
    // Release D3D12 resources
    m_swapChainBuffers.clear();
    if (m_fenceEvent)
    {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }
    m_fence.Reset();
    m_swapChain.Reset();
    m_commandQueue.Reset();
    m_d3d12Device.Reset();
    m_adapter.Reset();
    m_dxgiFactory.Reset();
    
    // Cleanup GLFW
    if (m_window)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

int main(int argc, char* argv[])
{
    TriangleApp app;
    
    if (!app.initialize())
    {
        std::cerr << "Failed to initialize application" << std::endl;
        return -1;
    }
    
    std::cout << "NVRHI Triangle Demo started. Press Escape or close window to exit." << std::endl;
    
    app.mainLoop();
    app.cleanup();
    
    return 0;
}
