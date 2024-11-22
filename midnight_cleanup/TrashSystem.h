#pragma once
#include "Animacore/SystemInterface.h"
#include "Animacore/SystemTraits.h"

namespace mc
{
	struct Quest;

	class TrashSystem : public core::ISystem, public core::IUpdateSystem, public core::ICollisionHandler
	{
	public:
		explicit TrashSystem(core::Scene& scene);
		~TrashSystem() override;

		void operator()(core::Scene& scene, float tick) override;

		void OnCollisionEnter(entt::entity self, entt::entity other, entt::registry& registry) override;
		void OnCollisionStay(entt::entity self, entt::entity other, entt::registry& registry) override;
		void OnCollisionExit(entt::entity self, entt::entity other, entt::registry& registry) override;

	private:
		void startSystem(const core::OnStartSystem& event);
		void finishSystem(const core::OnFinishSystem& event);

		entt::dispatcher* _dispatcher = nullptr;
		mc::Quest* _quest = nullptr;
	};
}

DEFINE_SYSTEM_TRAITS(mc::TrashSystem)

