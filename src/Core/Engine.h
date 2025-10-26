#pragma once

#define LOG_DEBUG
#include <tinyLog/Log.h>
#include <webgpu.h>

#define GLFW_INCLUDE_NONE
#include "GLFW\glfw3.h"

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h> // for glfwGetWin32Window()
#endif
#ifdef __APPLE__
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h> // for Cocoa window → CAMetalLayer
#endif

namespace PSB
{
	class Engine
	{
	public:
		void Init();
		void Shutdown();

	private:

	};
};
