#pragma once
#include <Animacore/SystemTraits.h>
#include <Animacore/SystemInterface.h>

namespace core
{
	struct OnStartSystem;
	struct OnFinishSystem;
}

namespace mc
{
	class CreditSceneSystem : public core::ISystem, public core::IFixedSystem
	{
	public:
		CreditSceneSystem(core::Scene& scene);
		~CreditSceneSystem() override;

		void operator()(core::Scene& scene) override;

	private:
		void startSystem(const core::OnStartSystem& event);
		void finishSystem(const core::OnFinishSystem& event);

	private:
		entt::dispatcher* _dispatcher = nullptr;
		float _timeElapsed = 0.0f;
		float _startPositionY = -1580.0f;
	};
}

DEFINE_SYSTEM_TRAITS(mc::CreditSceneSystem)