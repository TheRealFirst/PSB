#include "Engine.h"



void PSB::Engine::Init()
{
	initialize_logging();

	LOG_DEBUG("Hello World!");
}

void PSB::Engine::Shutdown()
{
	shutdown_logging();
}
