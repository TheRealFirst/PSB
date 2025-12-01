#pragma once

#include "Renderer\GraphicsContext.h"

namespace PSB
{
	class Renderer
	{
	public:
		Renderer();

		void Resize(uint32_t width);

	private:
		Ref<GraphicsContext> m_Context;
	};
}
