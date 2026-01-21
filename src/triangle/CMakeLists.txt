# NVRHI Triangle Demo CMakeLists.txt

set(TARGET_NAME triangle)

# Source files
set(SOURCES
    main.cpp
)

# Shader files (for IDE integration)
set(SHADERS
    shaders/triangle.slang
)

# Create executable
add_executable(${TARGET_NAME} ${SOURCES} ${SHADERS})

# Set source groups for IDE
source_group("Source Files" FILES ${SOURCES})
source_group("Shaders" FILES ${SHADERS})

# Link common library (includes nvrhi and backend dependencies)
target_link_libraries(${TARGET_NAME} PRIVATE common)

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
# Try Slang first, then fall back to DXC for DXIL
find_program(SLANGC_EXECUTABLE slangc)
find_program(DXC_EXECUTABLE dxc)

if(SLANGC_EXECUTABLE)
    message(STATUS "Found Slang compiler: ${SLANGC_EXECUTABLE}")
    
    # Custom target to compile shaders using Slang (both DXIL and SPIR-V)
    add_custom_target(compile_triangle_shaders
        # Compile DXIL shaders for D3D12
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
        # Compile SPIR-V shaders for Vulkan
        COMMAND ${SLANGC_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.slang"
            -profile glsl_450
            -target spirv
            -entry vsMain
            -stage vertex
            -o "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle_vs.spv"
        COMMAND ${SLANGC_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.slang"
            -profile glsl_450
            -target spirv
            -entry psMain
            -stage fragment
            -o "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle_ps.spv"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Compiling Slang shaders (DXIL + SPIR-V)..."
        SOURCES ${SHADERS}
    )
    
    # Make triangle depend on shader compilation
    add_dependencies(${TARGET_NAME} compile_triangle_shaders)
    
else()
    message(WARNING "No shader compiler found (slangc or dxc). Shaders must be compiled manually.")
    message(STATUS "To compile shaders manually:")
    message(STATUS "  For D3D12: slangc triangle.slang -profile sm_6_0 -target dxil -entry vsMain -stage vertex -o triangle_vs.dxil")
    message(STATUS "  For Vulkan: slangc triangle.slang -profile glsl_450 -target spirv -entry vsMain -stage vertex -o triangle_vs.spv")
    message(STATUS "Or run the compile_shaders.bat script in the shaders directory.")
endif()
