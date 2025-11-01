#include "Application.h"

namespace PSB
{
	Application* Application::s_Instance = nullptr;


	PSB::Application::Application()
	{
		initialize_logging();
		LOG_INFO("Starting up");

		s_Instance = this;

		m_Window = Window::Create();
		m_Window->SetEventCallback(PSB_BIND_EVENT_FN(Application::OnEvent));

	}

	PSB::Application::~Application()
	{
		LOG_INFO("Shutting down.")
		shutdown_logging();
	}

	void PSB::Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowResizeEvent>(PSB_BIND_EVENT_FN(Application::OnWindowResize));
		dispatcher.Dispatch<WindowCloseEvent>(PSB_BIND_EVENT_FN(Application::OnWindowClose));
	}

	void PSB::Application::Close()
	{
		m_Running = false;
	}

	void PSB::Application::Run()
	{
		while (m_Running)
		{
			if (!m_Minimized) {
				// Update
			}

			m_Window->OnUpdate();
		}
	}

	bool PSB::Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	bool PSB::Application::OnWindowResize(WindowResizeEvent& e)
	{
		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}

		m_Minimized = false;

		return false;
	}
}
