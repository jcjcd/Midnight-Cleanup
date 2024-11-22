#pragma once
#include "SystemTraits.h"
#include "SystemInterface.h"

namespace core
{
	class FreeFlyCameraSystem : public ISystem, public IUpdateSystem
	{
	public:
		FreeFlyCameraSystem(Scene& scene);
		~FreeFlyCameraSystem() override { _dispatcher->disconnect(this); }

		void operator()(Scene& scene, float tick) override;

	private:
		entt::dispatcher* _dispatcher = nullptr;
	};
}
DEFINE_SYSTEM_TRAITS(core::FreeFlyCameraSystem)
