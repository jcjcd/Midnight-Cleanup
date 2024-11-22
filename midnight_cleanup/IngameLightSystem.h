#pragma once
#include "Animacore/SystemInterface.h"
#include "Animacore/SystemTraits.h"

namespace  mc
{
	struct OnSwitchPushed;
	struct OnHandleFlashLight;

	class IngameLightSystem : public core::ISystem, public core::IRenderSystem
	{
	public:
		IngameLightSystem(core::Scene& scene);
		~IngameLightSystem() override;

		void operator()(core::Scene& scene, Renderer& renderer, float tick) override;

	private:
		void switchPushed(const mc::OnSwitchPushed& event);
		void handleFlashlight(const mc::OnHandleFlashLight& event);

	private:
		entt::dispatcher* _dispatcher = nullptr;
	};
}

DEFINE_SYSTEM_TRAITS(mc::IngameLightSystem)

