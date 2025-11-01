#include "Window.h"
#include "tinyLog\Log.h"
#include "tinyLog\Asserts.h"

#ifdef _WIN32
#include "Platform\Windows\WindowsWindow.h"
#endif

namespace PSB
{
	Scope<Window> Window::Create(const WindowProbs& probs)
	{
		#ifdef _WIN32
		return CreateScope<WindowsWindow>(probs);
		#else
		LOG_ASSERT(false, "Unsupported Platform!");
		return nullptr;
		#endif
	}
}