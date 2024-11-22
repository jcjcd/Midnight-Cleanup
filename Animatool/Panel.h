#pragma once
#include "ToolEvents.h"

namespace tool
{
	class Panel
	{
	public:
		Panel(entt::dispatcher& dispatcher) : _dispatcher(&dispatcher) {}
		virtual ~Panel() { _dispatcher->disconnect(this); }

		void BroadcastModifyScene() { _dispatcher->trigger<OnToolModifyScene>(); }
		virtual void RenderPanel(float deltaTime) = 0;
		virtual PanelType GetType() = 0;

	protected:
		entt::dispatcher* _dispatcher = nullptr;
		bool _isOpen = true;
	};
}

