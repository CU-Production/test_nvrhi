# Common library CMakeLists.txt
# Device management abstraction supporting D3D12 and Vulkan

set(TARGET_NAME common)

# Source files
set(SOURCES
    DeviceManager.cpp
    DeviceManager.h
    DeviceManager_VK.cpp
    DeviceManager_VK.h
)

# Add D3D12 sources on Windows
if(WIN32)
    list(APPEND SOURCES
        DeviceManager_D3D12.cpp
        DeviceManager_D3D12.h
    )
endif()

# Create static library
add_library(${TARGET_NAME} STATIC ${SOURCES})

# Include directories
target_include_directories(${TARGET_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/3rd_party/NVRHI/include
    ${CMAKE_SOURCE_DIR}/3rd_party/glfw/include
)

# Link libraries
target_link_libraries(${TARGET_NAME} PUBLIC
    nvrhi
    nvrhi_vk
    glfw
    Vulkan::Headers
)

# Platform specific libraries
if(WIN32)
    target_link_libraries(${TARGET_NAME} PUBLIC
        nvrhi_d3d12
        d3d12
        dxgi
        dxguid
    )
    
    target_compile_definitions(${TARGET_NAME} PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
        VK_USE_PLATFORM_WIN32_KHR
    )
endif()

# Set C++ standard
target_compile_features(${TARGET_NAME} PUBLIC cxx_std_20)
