#pragma once

#include "Window.h"
#include "Events\Event.h"
#include "Events\ApplicationEvent.h"

#include "Core.h"
#include "Layerstack.h"

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

int main(int argc, char** argv);

namespace PSB
{
	class Application
	{
	public:
		Application();
		virtual ~Application();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

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

		float m_LastFrameTime = 0.0f;

		LayerStack m_LayerStack;
	private:
		static Application* s_Instance;
		friend int ::main(int argc, char** argv);
	};

	Application* CreateApplication();
}