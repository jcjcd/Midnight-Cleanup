#pragma once
#include <Animacore/SystemInterface.h>

#include "Animacore/SystemTraits.h"

namespace mc
{
	class DropSoundSystem : public core::ISystem, public core::ICollisionHandler
	{
	public:
		explicit DropSoundSystem(core::Scene& scene);
		~DropSoundSystem() override;

		void OnCollisionEnter(entt::entity self, entt::entity other, entt::registry& registry) override;
		void OnCollisionStay(entt::entity self, entt::entity other, entt::registry& registry) override;
		void OnCollisionExit(entt::entity self, entt::entity other, entt::registry& registry) override;

	private:
		void startSystem(const core::OnStartSystem& event);
		void finishSystem(const core::OnFinishSystem& event);

	
		entt::dispatcher* _dispatcher = nullptr;
	};
}
DEFINE_SYSTEM_TRAITS(mc::DropSoundSystem)
