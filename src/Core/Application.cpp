#include "Application.h"

namespace PSB
{
	Application* Application::s_Instance = nullptr;


	PSB::Application::Application()
	{
		initialize_logging();
		LOG_INFO("Starting up");

		s_Instance = this;

		WindowProbs probs;

		m_Window = Window::Create();
		m_Context = m_Window->GetGraphicsContext();
		m_Window->SetEventCallback(PSB_BIND_EVENT_FN(Application::OnEvent));

		m_GuiLayer = new GUI::GUILayer();
		m_GuiLayer->Init(m_Window->GetWidth(), m_Window->GetHeight());
		PushOverlay(m_GuiLayer);
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

		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); it++)
		{
			if (e.Handled)
				break;
			(*it)->OnEvent(e);
		}
	}

	void PSB::Application::Close()
	{
		m_Running = false;
	}

	void PSB::Application::Run()
	{
		while (m_Running)
		{
			float time = (float)glfwGetTime();
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			if (!m_Minimized) {
				for (Layer* layer : m_LayerStack)
					layer->OnUpdate(timestep);
			}

			m_Window->OnUpdate();
		}
	}

	void PSB::Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void PSB::Application::PushOverlay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
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

		m_Window->GetGraphicsContext()->OnWindowResize(e.GetWidth(), e.GetHeight());
		m_GuiLayer->OnResize(e.GetWidth(), e.GetHeight());
		m_Minimized = false;

		return false;
	}
}
