#pragma once

#include "Window.h"
#include "Events\Event.h"
#include "Events\ApplicationEvent.h"

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
	class Application
	{
	public:
		Application();
		virtual ~Application();

		void OnEvent(Event& e);

		Window& GetWindow() { return *m_Window; }

		void Close();

		static Application& Get()
		{
			return *s_Instance;
		}
	private:
		void Run();
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

	private:
		Scope<Window> m_Window;
		bool m_Running = true;
		bool m_Minimized = false;
	private:
		static Application* s_Instance;
	};
}