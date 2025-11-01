# PSB/CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(PSB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(dependencies/tinyLog)


# Build only the library
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
# Build static (recommended) – set ON to get glfw3.dll instead
set(BUILD_SHARED_LIBS   OFF CACHE BOOL "" FORCE)

add_subdirectory(dependencies/glfw)  # defines target: glfw
add_subdirectory(dependencies/glm)

set(WGPU_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/wgpu" CACHE PATH "Path to wgpu-native package (include/lib/bin)")

add_library(wgpu_native SHARED IMPORTED GLOBAL)

if(WIN32)
  # MSVC or MinGW: both use .dll + .lib import library
  set_target_properties(wgpu_native PROPERTIES
    IMPORTED_LOCATION     "${WGPU_DIR}/bin/wgpu_native.dll"
    IMPORTED_IMPLIB       "${WGPU_DIR}/lib/wgpu_native.lib"
  )
elseif(APPLE)
  set_target_properties(wgpu_native PROPERTIES
    IMPORTED_LOCATION     "${WGPU_DIR}/lib/libwgpu_native.dylib"
  )
else()
  # If you also target Linux later:
  # set_target_properties(wgpu_native PROPERTIES
  #   IMPORTED_LOCATION   "${WGPU_DIR}/lib/libwgpu_native.so"
  # )
endif()

target_include_directories(wgpu_native INTERFACE "${WGPU_DIR}/include")

file(GLOB_RECURSE PSB_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

add_library(PSB STATIC ${PSB_SOURCES} "src/Core/Window.h" "src/Core/Window.cpp" "src/Core/Input.h" "src/Core/MouseCodes.h" "src/Core/KeyCodes.h" "src/Events/ApplicationEvent.h" "src/Events/KeyEvent.h" "src/Events/MouseEvent.h" "src/Platform/Windows/WindowsWindow.h" "src/Platform/Windows/WindowsInput.cpp" "src/Core/Application.h" "src/Renderer/GraphicsContext.h" "src/Renderer/GraphicsContext.cpp" "src/Core/Application.cpp")

target_include_directories(PSB
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link deps
target_link_libraries(PSB
    PUBLIC
        tinyLog
        wgpu_native
	glfw
	glm
)        

# Define platform-specific preprocessor symbols for all code that includes PSB
if(WIN32)
    target_compile_definitions(PSB PUBLIC PSB_WIN32)
elseif(APPLE)
    target_compile_definitions(PSB PUBLIC PSB_MACOS)
elseif(UNIX)
    target_compile_definitions(PSB PUBLIC PSB_LINUX)
endif()

function(PSB_copy_wgpu_runtime target)
  if(WIN32)
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "$<TARGET_FILE:wgpu_native>"            # resolves to the DLL
              "$<TARGET_FILE_DIR:${target}>"
      COMMENT "[PSB] Copied wgpu_native runtime to $<TARGET_FILE_DIR:${target}>")
  elseif(APPLE)
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "$<TARGET_FILE:wgpu_native>"            # resolves to the dylib
              "$<TARGET_FILE_DIR:${target}>"
      COMMENT "[PSB] Copied wgpu_native runtime to $<TARGET_FILE_DIR:${target}>")
  endif()
endfunction()
