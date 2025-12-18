cmake_minimum_required(VERSION 3.20)
project(PSB LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(dependencies/tinyLog)
add_subdirectory(dependencies/webgpu)

# GLFW static
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS   OFF CACHE BOOL "" FORCE)
add_subdirectory(dependencies/glfw)  # target: glfw



# Sources
file(GLOB_RECURSE PSB_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

add_library(PSB STATIC ${PSB_SOURCES} "src/Core/Layer.h" "src/Core/Layer.cpp" "src/Core/Timestep.h" "src/Core/Layerstack.h" "src/Core/Layerstack.cpp" "src/Core/EntryPoint.h" "src/Renderer/Renderer.h" "src/Renderer/Renderer.cpp" "src/Renderer/Renderer2D.h" "src/Renderer/Renderer2D.cpp" "src/Renderer/Buffers/VertexBuffer.h" "src/Renderer/Buffers/VertexBuffer.cpp" "src/Renderer/Buffers/IndexBuffer.h" "src/Renderer/Buffers/IndexBuffer.cpp" "src/AssetManagement/AssetManager.h" "src/AssetManagement/AssetManager.cpp" "src/Renderer/RendererUtils.h" "src/Renderer/RendererUtils.cpp" "dependencies/TinyOBJLoader/tiny_obj_loader.h")

target_include_directories(PSB PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glm   # glm is header-only
  ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glfw3webgpu/include
  ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/TinyOBJLoader)




add_subdirectory(dependencies/glfw3webgpu)

# Core deps for consumers (Sample inherits these)
target_link_libraries(PSB PUBLIC tinyLog webgpu glfw glfw3webgpu)
# target_copy_webgpu_binaries(PSB)

target_compile_definitions(PSB PUBLIC WEBGPU_BACKEND_WGPU=1)
if (WIN32)
  target_compile_definitions(glfw3webgpu PRIVATE GLFW_EXPOSE_NATIVE_WIN32=1)
endif()

# Platform defines
if (WIN32)
  target_compile_definitions(PSB PUBLIC PSB_WIN32)
elseif(APPLE)
  target_compile_definitions(PSB PUBLIC PSB_MACOS)
elseif(UNIX)
  target_compile_definitions(PSB PUBLIC PSB_LINUX)
endif()

# Windows system libs needed by wgpu_native
if (WIN32)
  target_link_libraries(PSB PUBLIC
    user32 gdi32 shell32 ole32 oleaut32 uuid advapi32
    userenv        # GetUserProfileDirectoryW
    ntdll          # Nt* + RtlNtStatusToDosError
    ws2_32         # Winsock
    propsys        # VariantToPropVariant, PropVariantToBSTR
    runtimeobject  # RoOriginateErrorW
    opengl32       # wgl* referenced by GLES path
  )
endif()



option(DEV_MODE "Set up development helper settings" ON)

if(DEV_MODE)
    # In dev mode, we load resources from the source tree, so that when we
    # dynamically edit resources (like shaders), these are correctly
    # versionned.
    target_compile_definitions(PSB PRIVATE
        RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
    )
else()
    # In release mode, we just load resources relatively to wherever the
    # executable is launched from, so that the binary is portable
    target_compile_definitions(PSB PRIVATE
        RESOURCE_DIR="./resources"
    )
endif()

TARGET_COPY_WEBGPU_BINARIES(PSB)

function(PSB_copy_wgpu_runtime target)
  if(WIN32 OR APPLE)
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "$<TARGET_FILE:webgpu>"
              "$<TARGET_FILE_DIR:${target}>")
  endif()
endfunction()
