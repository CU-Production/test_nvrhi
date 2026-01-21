# NVRHI
add_subdirectory(NVRHI)

# pugiXML
add_subdirectory(pugixml)

# GLFW
set(GLFW_BUILD_DOCS OFF)
add_subdirectory(glfw)

# stb
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE stb)

# tinyobj
add_library(tinyobj INTERFACE)
target_include_directories(tinyobj INTERFACE tinyobjloader-c)

# tinyexr
add_library(tinyexr tinyexr/miniz/miniz.c)
target_include_directories(tinyexr PUBLIC tinyexr tinyexr/miniz)
