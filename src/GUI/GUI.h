#pragma once

#include "core/Layer.h"
#include <vector>

#include "GUIItem.h"

namespace PSB::GUI
{
	class GUILayer : public Layer
	{
	public:
		GUILayer() = default;
		~GUILayer() = default;

		void OnEvent(Event& e);

	private:
		std::vector<GUIItem> m_Items;
	};
}