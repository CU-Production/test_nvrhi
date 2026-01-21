// NVRHI Triangle Demo
// This demo shows how to render a simple colored triangle using NVRHI
// Supports both D3D12 and Vulkan backends

#include <DeviceManager.h>

#include <GLFW/glfw3.h>

#include <nvrhi/nvrhi.h>
#include <nvrhi/utils.h>

#include <iostream>
#include <vector>
#include <array>
#include <fstream>
#include <memory>

// Window dimensions
constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

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

// Application class encapsulating all rendering state
class TriangleApp
{
public:
    bool initialize(common::GraphicsAPI api);
    void mainLoop();
    void cleanup();

private:
    bool initWindow();
    bool loadShaders();
    bool createPipeline();
    bool createVertexBuffer();
    
    void render();
    void onResize(int width, int height);
    
    std::vector<uint8_t> loadShaderFromFile(const std::string& filename);

    // GLFW window resize callback
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

private:
    // Window
    GLFWwindow* m_window = nullptr;
    int m_windowWidth = WINDOW_WIDTH;
    int m_windowHeight = WINDOW_HEIGHT;
    bool m_windowResized = false;
    
    // Device manager (handles D3D12/Vulkan backend)
    std::unique_ptr<common::IDeviceManager> m_deviceManager;
    nvrhi::CommandListHandle m_commandList;
    
    // Pipeline resources
    nvrhi::ShaderHandle m_vertexShader;
    nvrhi::ShaderHandle m_pixelShader;
    nvrhi::InputLayoutHandle m_inputLayout;
    nvrhi::GraphicsPipelineHandle m_pipeline;
    nvrhi::BufferHandle m_vertexBuffer;
};

void TriangleApp::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    auto app = static_cast<TriangleApp*>(glfwGetWindowUserPointer(window));
    app->m_windowWidth = width;
    app->m_windowHeight = height;
    app->m_windowResized = true;
}

bool TriangleApp::initialize(common::GraphicsAPI api)
{
    if (!initWindow()) return false;
    
    // Create device manager for the selected API
    m_deviceManager = common::createDeviceManager(api);
    if (!m_deviceManager)
    {
        std::cerr << "Failed to create device manager for " << common::graphicsAPIToString(api) << std::endl;
        return false;
    }
    
    // Set up device creation params
    common::DeviceCreationParams params;
    params.window = m_window;
    params.windowWidth = m_windowWidth;
    params.windowHeight = m_windowHeight;
    params.swapChainBufferCount = 2;
    params.enableDebugLayer = true;
    params.enableValidationLayer = true;
    params.vsync = true;
    
    if (!m_deviceManager->createDevice(params))
    {
        std::cerr << "Failed to create device" << std::endl;
        return false;
    }
    
    // Create command list
    m_commandList = m_deviceManager->createCommandList();
    if (!m_commandList)
    {
        std::cerr << "Failed to create command list" << std::endl;
        return false;
    }
    
    if (!loadShaders()) return false;
    if (!createPipeline()) return false;
    if (!createVertexBuffer()) return false;
    
    std::cout << "Initialized with " << m_deviceManager->getGraphicsAPIName() << " backend" << std::endl;
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
    
    // Set user pointer for resize callback
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    
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
    // Determine shader file extension based on API
    std::string vsFile, psFile;
    
    if (m_deviceManager->getGraphicsAPI() == common::GraphicsAPI::D3D12)
    {
        vsFile = "shaders/triangle_vs.dxil";
        psFile = "shaders/triangle_ps.dxil";
    }
    else // Vulkan
    {
        vsFile = "shaders/triangle_vs.spv";
        psFile = "shaders/triangle_ps.spv";
    }
    
    std::vector<uint8_t> vsData = loadShaderFromFile(vsFile);
    std::vector<uint8_t> psData = loadShaderFromFile(psFile);
    
    if (vsData.empty() || psData.empty())
    {
        std::cerr << "Failed to load shader files. Please compile shaders first." << std::endl;
        std::cerr << "Required files: " << vsFile << ", " << psFile << std::endl;
        std::cerr << std::endl;
        std::cerr << "For D3D12 (DXIL), run:" << std::endl;
        std::cerr << "  slangc triangle.slang -profile sm_6_0 -target dxil -entry vsMain -stage vertex -o triangle_vs.dxil" << std::endl;
        std::cerr << "  slangc triangle.slang -profile sm_6_0 -target dxil -entry psMain -stage fragment -o triangle_ps.dxil" << std::endl;
        std::cerr << std::endl;
        std::cerr << "For Vulkan (SPIR-V), run:" << std::endl;
        std::cerr << "  slangc triangle.slang -profile glsl_450 -target spirv -entry vsMain -stage vertex -o triangle_vs.spv" << std::endl;
        std::cerr << "  slangc triangle.slang -profile glsl_450 -target spirv -entry psMain -stage fragment -o triangle_ps.spv" << std::endl;
        return false;
    }
    
    // Entry point names differ between D3D12 (keeps original name) and Vulkan (SPIR-V uses "main")
    const char* vsEntryName = (m_deviceManager->getGraphicsAPI() == common::GraphicsAPI::Vulkan) ? "main" : "vsMain";
    const char* psEntryName = (m_deviceManager->getGraphicsAPI() == common::GraphicsAPI::Vulkan) ? "main" : "psMain";
    
    // Create vertex shader
    nvrhi::ShaderDesc vsDesc = {};
    vsDesc.shaderType = nvrhi::ShaderType::Vertex;
    vsDesc.debugName = "TriangleVS";
    vsDesc.entryName = vsEntryName;
    
    m_vertexShader = m_deviceManager->getDevice()->createShader(vsDesc, vsData.data(), vsData.size());
    if (!m_vertexShader)
    {
        std::cerr << "Failed to create vertex shader" << std::endl;
        return false;
    }
    
    // Create pixel shader
    nvrhi::ShaderDesc psDesc = {};
    psDesc.shaderType = nvrhi::ShaderType::Pixel;
    psDesc.debugName = "TrianglePS";
    psDesc.entryName = psEntryName;
    
    m_pixelShader = m_deviceManager->getDevice()->createShader(psDesc, psData.data(), psData.size());
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
    
    m_inputLayout = m_deviceManager->getDevice()->createInputLayout(
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
    fbInfo.addColorFormat(m_deviceManager->getSwapChainFormat());
    
    m_pipeline = m_deviceManager->getDevice()->createGraphicsPipeline(pipelineDesc, fbInfo);
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
    
    m_vertexBuffer = m_deviceManager->getDevice()->createBuffer(bufferDesc);
    if (!m_vertexBuffer)
    {
        std::cerr << "Failed to create vertex buffer" << std::endl;
        return false;
    }
    
    // Upload vertex data
    m_commandList->open();
    m_commandList->writeBuffer(m_vertexBuffer, g_TriangleVertices.data(), bufferDesc.byteSize);
    m_commandList->close();
    
    m_deviceManager->executeCommandList(m_commandList);
    m_deviceManager->waitForIdle();
    
    return true;
}

void TriangleApp::onResize(int width, int height)
{
    if (width == 0 || height == 0)
        return;
    
    m_deviceManager->resizeSwapChain(width, height);
}

void TriangleApp::render()
{
    // Handle window resize
    if (m_windowResized)
    {
        onResize(m_windowWidth, m_windowHeight);
        m_windowResized = false;
    }
    
    // Skip rendering if window is minimized
    if (m_windowWidth == 0 || m_windowHeight == 0)
        return;
    
    // Begin frame (acquires next swap chain image)
    m_deviceManager->beginFrame();
    
    // Begin recording commands
    m_commandList->open();
    
    // Clear render target to dark blue
    nvrhi::utils::ClearColorAttachment(m_commandList, m_deviceManager->getCurrentFramebuffer(), 0,
        nvrhi::Color(0.1f, 0.1f, 0.2f, 1.0f));
    
    // Set up graphics state
    nvrhi::GraphicsState state = {};
    state.pipeline = m_pipeline;
    state.framebuffer = m_deviceManager->getCurrentFramebuffer();
    state.viewport.addViewportAndScissorRect(nvrhi::Viewport(
        static_cast<float>(m_deviceManager->getWindowWidth()),
        static_cast<float>(m_deviceManager->getWindowHeight())));
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
    m_deviceManager->executeCommandList(m_commandList);
    
    // Present
    m_deviceManager->present();
}

void TriangleApp::mainLoop()
{
    while (!glfwWindowShouldClose(m_window))
    {
        if(glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(m_window, true);
        glfwPollEvents();
        render();
    }
    
    // Wait for GPU before cleanup
    m_deviceManager->waitForIdle();
}

void TriangleApp::cleanup()
{
    // Release pipeline resources
    m_vertexBuffer = nullptr;
    m_pipeline = nullptr;
    m_inputLayout = nullptr;
    m_pixelShader = nullptr;
    m_vertexShader = nullptr;
    m_commandList = nullptr;
    
    // Destroy device manager
    if (m_deviceManager)
    {
        m_deviceManager->destroyDevice();
        m_deviceManager.reset();
    }
    
    // Cleanup GLFW
    if (m_window)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

// Parse command line arguments to select graphics API
common::GraphicsAPI parseCommandLine(int argc, char* argv[])
{
    // Default to D3D12 on Windows, Vulkan otherwise
#ifdef _WIN32
    common::GraphicsAPI defaultAPI = common::GraphicsAPI::D3D12;
#else
    common::GraphicsAPI defaultAPI = common::GraphicsAPI::Vulkan;
#endif
    
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "-d3d12" || arg == "--d3d12" || arg == "-dx12")
        {
            return common::GraphicsAPI::D3D12;
        }
        else if (arg == "-vulkan" || arg == "--vulkan" || arg == "-vk")
        {
            return common::GraphicsAPI::Vulkan;
        }
        else if (arg == "-h" || arg == "--help")
        {
            std::cout << "NVRHI Triangle Demo" << std::endl;
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -d3d12, --d3d12, -dx12    Use D3D12 backend (Windows only)" << std::endl;
            std::cout << "  -vulkan, --vulkan, -vk    Use Vulkan backend" << std::endl;
            std::cout << "  -h, --help                Show this help message" << std::endl;
            std::exit(0);
        }
    }
    
    return defaultAPI;
}

int main(int argc, char* argv[])
{
    common::GraphicsAPI api = parseCommandLine(argc, argv);
    
    std::cout << "NVRHI Triangle Demo" << std::endl;
    std::cout << "Selected API: " << common::graphicsAPIToString(api) << std::endl;
    
    TriangleApp app;
    
    if (!app.initialize(api))
    {
        std::cerr << "Failed to initialize application" << std::endl;
        return -1;
    }
    
    std::cout << "Press Escape or close window to exit." << std::endl;
    
    app.mainLoop();
    app.cleanup();
    
    return 0;
}
