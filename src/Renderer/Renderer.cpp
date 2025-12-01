#include "Renderer.h"
#include "Core\Application.h"

namespace PSB
{
	Renderer::Renderer()
	{
		m_Context = Application::Get().GetContext();
	}
}