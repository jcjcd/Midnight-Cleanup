#pragma once
#include <random>
#include <Animacore/SystemTraits.h>
#include <Animacore/SystemInterface.h>

namespace core
{
	struct OnStartSystem;
	struct OnFinishSystem;
	struct WorldTransform;
	struct LocalTransform;
}

namespace mc
{
	struct Quest;
	struct RayCastingInfo;
	struct OnProcessFurniture;
	
	class FurnitureDiscardingSystem : public core::ISystem, public core::IRenderSystem
	{
	public:
		FurnitureDiscardingSystem(core::Scene& scene);
		~FurnitureDiscardingSystem() override;

		void operator()(core::Scene& scene, Renderer& renderer, float tick) override;

	private:
		void startSystem(const core::OnStartSystem& event);
		void finishSystem(const core::OnFinishSystem& event);
		void applyCameraShake(core::Scene& scene, core::LocalTransform& cameraLocal, float tick, float amplitude, float duration);
		void processRissolveEffect(core::Scene& scene, float tick);

		void processFurniture(const mc::OnProcessFurniture& event);


	private:
		std::mt19937* _generator = nullptr;
		mc::RayCastingInfo* _rayCastingInfo = nullptr;
		entt::dispatcher* _dispatcher = nullptr;
		mc::Quest* _quest = nullptr;

		entt::entity _axe = entt::null;
		entt::entity _player = entt::null;
		entt::entity _playerCamera = entt::null;
		entt::entity _curParticle = entt::null;
		float _particleLifeTime = 0.0f;

		Vector3 _localCamPos;
		Quaternion _localCamRot;
		Vector3 _localCamScale;
		float _timeElapsed = 0.f;
		bool _isHit = false;
		bool _isOrigin = true;
	};
}

DEFINE_SYSTEM_TRAITS(mc::FurnitureDiscardingSystem)