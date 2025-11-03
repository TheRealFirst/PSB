#pragma once

#ifdef _WIN32

extern PSB::Application* PSB::CreateApplication();

int main(int argc, char** argv)
{
	auto app = Engine::CreateApplication();
	app->Run();
	delete app;
}

#else
#error Other Platforms not yet supported
#endif