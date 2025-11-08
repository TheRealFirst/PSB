#pragma once

#ifdef _WIN32

extern PSB::Application* PSB::CreateApplication();

int main(int argc, char** argv)
{
	auto app = PSB::CreateApplication();
	app->Run();
	delete app;
}

#else
#error Other Platforms not yet supported
#endif