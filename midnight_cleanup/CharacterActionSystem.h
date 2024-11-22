#pragma once

#include <Animacore/SystemTraits.h>
#include <Animacore/SystemInterface.h>

#include "McComponents.h"
#include "Animacore/CoreComponents.h"

namespace core
{
	struct OnStartSystem;
	struct OnFinishSystem;

	struct WorldTransform;
	struct Animator;
}


namespace mc
{
	struct OnDiscardTool;
	struct RayCastingInfo;
	struct Inventory;
	struct OnProcessWipe;

	class CharacterActionSystem : public core::IPreUpdateSystem, public core::ISystem, public core::IUpdateSystem
	{
	public:
		CharacterActionSystem(core::Scene& scene);
		~CharacterActionSystem();

		void PreUpdate(core::Scene& scene, float tick) override;
		void operator()(core::Scene& scene, float tick) override;
		void pickUpTool(core::Scene& scene, const core::Input& input, mc::Inventory& inventory);
		void handleToolInteractions(core::Scene& scene, const core::Input& input, entt::entity entity, mc::Inventory& inventory, Tool::Type curToolType, float tick);
		void repairTool(core::Scene& scene, mc::Inventory& inventory, entt::entity hitEntity);
		void interactWithStain(core::Scene& scene, mc::Inventory& inventory, entt::entity hitEntity);
		void cleanStain(core::Scene& scene, mc::Inventory& inventory, entt::entity hitEntity);
		void updateMopMaterial(core::Scene& scene, mc::Inventory& inventory, Durability* durability, float prevPercentage);
		void updateSpongeMaterial(core::Scene& scene, mc::Inventory& inventory, Durability* durability, float prevPercentage);
		void endInteraction(core::Scene& scene);
		void continueInteraction(core::Scene& scene, const core::Input& input, mc::Inventory& inventory, float tick);
		void createNewStain(core::Scene& scene);

	private:
		void startSystem(const core::OnStartSystem& event);
		void finishSystem(const core::OnFinishSystem& event);

		void handleRaycastInteraction(const mc::OnProcessWipe& event);
		void discardTool(const mc::OnDiscardTool& event);

	private:
		mc::RayCastingInfo* _rayCastingInfo = nullptr;
		entt::dispatcher* _dispatcher = nullptr;

		entt::entity _pivotEntity = entt::null;

		bool _isInteracting = false;
		entt::entity _interactingEntity = entt::null;
		core::WorldTransform* _rayCastingTransform = nullptr;
		core::Animator* _animator = nullptr;
		float _accumulator = 0.f;
		mc::Inventory* _inventory = nullptr;
		entt::entity _flashlight = entt::null;

		bool _holdMouse = false;

		// 스폰지 상호작용을 위한 변수
		float _interactingOffsetX = 0.f;
		float _interactingOffsetY = 0.f;

		// 내부적으로 클릭 처리 분기를 위한 변수
		bool _isPickedUp = false;

		// particle
		entt::entity _mopParticle = entt::null;
		float _particleLifeTime = 0.0f;

		// 플레이어
		entt::entity _player = entt::null;
	};
}
DEFINE_SYSTEM_TRAITS(mc::CharacterActionSystem)
