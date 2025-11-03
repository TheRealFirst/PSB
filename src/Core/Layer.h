#pragma once

#include "Core.h"
#include "Timestep.h"
#include "Events\Event.h"

namespace PSB
{
	class Layer {
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer();

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(Timestep ts) {}
		virtual void OnEvent(Event& e){}

		const std::string& GetName() const { return m_DebugName; }

	protected:
		std::string m_DebugName;
	};
}