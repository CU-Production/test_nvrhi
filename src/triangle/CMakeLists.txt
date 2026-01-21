# NVRHI Triangle Demo CMakeLists.txt

set(TARGET_NAME triangle)

# Source files
set(SOURCES
    main.cpp
)

# Shader files (for IDE integration)
set(SHADERS
    shaders/triangle.slang
    shaders/triangle.hlsl
)

# Create executable
add_executable(${TARGET_NAME} ${SOURCES} ${SHADERS})

# Set source groups for IDE
source_group("Source Files" FILES ${SOURCES})
source_group("Shaders" FILES ${SHADERS})

# Include directories
target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/3rd_party/NVRHI/include
    ${CMAKE_SOURCE_DIR}/3rd_party/glfw/include
)

# Link libraries
target_link_libraries(${TARGET_NAME} PRIVATE
    nvrhi
    nvrhi_d3d12
    glfw
    d3d12
    dxgi
    dxguid
)

# Windows-specific settings
if(WIN32)
    target_compile_definitions(${TARGET_NAME} PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
    )
    
    # Link Windows libraries
    target_link_libraries(${TARGET_NAME} PRIVATE
        dwmapi
    )
endif()

# Set working directory for debugging
set_target_properties(${TARGET_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Copy shader files to output directory
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${TARGET_NAME}>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:${TARGET_NAME}>/shaders"
    COMMENT "Copying shader files..."
)

# Optional: Add shader compilation target
# Try Slang first, then fall back to DXC
find_program(SLANGC_EXECUTABLE slangc)
find_program(DXC_EXECUTABLE dxc)

if(SLANGC_EXECUTABLE)
    message(STATUS "Found Slang compiler: ${SLANGC_EXECUTABLE}")
    
    # Custom target to compile shaders using Slang
    add_custom_target(compile_triangle_shaders
        COMMAND ${SLANGC_EXECUTABLE} 
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.slang"
            -profile sm_6_0
            -target dxil
            -entry vsMain
            -stage vertex
            -o "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle_vs.dxil"
        COMMAND ${SLANGC_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.slang"
            -profile sm_6_0
            -target dxil
            -entry psMain
            -stage fragment
            -o "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle_ps.dxil"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Compiling Slang shaders..."
        SOURCES ${SHADERS}
    )
    
    # Make triangle depend on shader compilation
    add_dependencies(${TARGET_NAME} compile_triangle_shaders)
    
elseif(DXC_EXECUTABLE)
    message(STATUS "Found DXC compiler: ${DXC_EXECUTABLE}")
    
    # Custom target to compile shaders using DXC
    add_custom_target(compile_triangle_shaders
        COMMAND ${DXC_EXECUTABLE}
            -T vs_6_0
            -E vsMain
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.hlsl"
            -Fo "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle_vs.dxil"
        COMMAND ${DXC_EXECUTABLE}
            -T ps_6_0
            -E psMain
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.hlsl"
            -Fo "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle_ps.dxil"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Compiling HLSL shaders with DXC..."
        SOURCES ${SHADERS}
    )
    
    # Make triangle depend on shader compilation
    add_dependencies(${TARGET_NAME} compile_triangle_shaders)
    
else()
    message(WARNING "No shader compiler found (slangc or dxc). Shaders must be compiled manually.")
    message(STATUS "To compile shaders manually, run one of:")
    message(STATUS "  Using Slang: slangc triangle.slang -profile sm_6_0 -target dxil -entry vsMain -stage vertex -o triangle_vs.dxil")
    message(STATUS "  Using DXC:   dxc -T vs_6_0 -E vsMain triangle.hlsl -Fo triangle_vs.dxil")
    message(STATUS "Or run the compile_shaders.bat script in the shaders directory.")
endif()
