#pragma once
#include <Animacore/SystemInterface.h>
#include <Animacore/SystemTraits.h>

namespace core
{
	class PhysicsScene;
	struct OnStartSystem;
	struct OnFinishSystem;
}

namespace mc
{
	struct RayCastingInfo;
	struct Picking;
	struct Inventory;
	struct OnNotifyToolChanged;

	class PickingSystem : public core::ISystem, public core::IUpdateSystem, public core::IFixedSystem
	{
	public:
		PickingSystem(core::Scene& scene);
		~PickingSystem() override;

		void operator()(core::Scene& scene, float tick) override;
		void operator()(core::Scene& scene) override;

	private:
		void pickingUpdate(core::Scene& scene, float tick);
		void snapUpdate(entt::registry& registry);
		void fillBucket(core::Scene& scene, entt::entity bucketEntity);

		void startSystem(const core::OnStartSystem& event);
		void finishSystem(const core::OnFinishSystem& event);

		void releaseItem(core::Scene& scene);
		void onNotifyToolChanged(const OnNotifyToolChanged& event);


		constexpr static char _waterMaterialName[] = "./Resources/Materials/Water_v1.material";


		std::shared_ptr<core::PhysicsScene> _physicsScene;
		entt::dispatcher* _dispatcher = nullptr;

		entt::entity _player = entt::null;
		Picking* _picking = nullptr;
		Inventory* _inventory = nullptr;
		RayCastingInfo* _ray = nullptr;

		float _timeElapsed = 0.0f;
		bool _isWorking = false;
	};
}

DEFINE_SYSTEM_TRAITS(mc::PickingSystem)