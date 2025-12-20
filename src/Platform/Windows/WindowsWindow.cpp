#include "WindowsWindow.h"
#include "Events\Event.h"
#include "Events\ApplicationEvent.h"
#include "Events\KeyEvent.h"
#include "Events\MouseEvent.h"
#include "Core\Application.h"

static uint8_t s_GLFWWindowCount = 0;

static void GLFWErrorCallback(int error, const char* description)
{
	LOG_ERROR("GLFW Error");
	LOG_ERROR(description);
}

PSB::WindowsWindow::WindowsWindow(const WindowProbs& probs)
{
    m_Context = Application::Get().GetContext();
    Init(probs);
}

PSB::WindowsWindow::~WindowsWindow()
{
    Shutdown();
}

void PSB::WindowsWindow::OnUpdate()
{
    glfwPollEvents();
    m_Context->OnFrame();
}

void PSB::WindowsWindow::SetVSync(bool enabled)
{
}

bool PSB::WindowsWindow::IsVSync() const
{
	return false;
}

void PSB::WindowsWindow::Init(const WindowProbs& probs)
{
	m_Data.Title = probs.Title;
	m_Data.Width = probs.Width;
	m_Data.Height = probs.Height;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	if (s_GLFWWindowCount == 0)
	{
		int success = glfwInit();
		LOG_ASSERT(success, "Could not initialize GLFW!");
		glfwSetErrorCallback(GLFWErrorCallback);
	}

	m_Window = glfwCreateWindow((int)probs.Width, (int)probs.Height, probs.Title.c_str(), nullptr, nullptr);
	++s_GLFWWindowCount;

	m_Context = CreateScope<GraphicsContext>(m_Window);
	LOG_ASSERT(m_Context->Init(m_Data.Width, m_Data.Height));


	if (m_Window == NULL)
	{
		LOG_FATAL("Failed Creating GLFWWindow");
		glfwTerminate();
		LOG_ASSERT(false);
	}

    glfwSetWindowUserPointer(m_Window, &m_Data);
    SetVSync(m_Data.VSync);


    // Set GLFW callbacks
    glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
            data.Width = width;
            data.Height = height;

            WindowResizeEvent event(width, height);
            data.EventCallback(event);
        });

    glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
            WindowCloseEvent event;
            data.EventCallback(event);
        });

    glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

            switch (action)
            {
            case GLFW_PRESS:
            {
                KeyPressedEvent event(key, 0);
                data.EventCallback(event);
                break;
            }
            case GLFW_RELEASE:
            {
                KeyReleasedEvent event(key);
                data.EventCallback(event);
                break;
            }
            case GLFW_REPEAT:
            {
                KeyPressedEvent event(key, 1);
                data.EventCallback(event);
                break;
            }
            }
        });

    glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

            KeyTypedEvent event(keycode);
            data.EventCallback(event);
        });

    glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

            switch (action)
            {
            case GLFW_PRESS:
            {
                MouseButtonPressedEvent event(button);
                data.EventCallback(event);
                break;
            }
            case GLFW_RELEASE:
            {
                MouseButtonReleasedEvent event(button);
                data.EventCallback(event);
                break;
            }
            }
        });

    glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

            MouseScrolledEvent event((float)xOffset, (float)yOffset);
            data.EventCallback(event);
        });

    glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
        {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

            MouseMovedEvent event((float)xPos, (float)yPos);
            data.EventCallback(event);
        });
}

void PSB::WindowsWindow::Shutdown()
{
	glfwDestroyWindow(m_Window);
	glfwTerminate();
	m_Context->Delete();
}
