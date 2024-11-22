#pragma once
#include <Animacore/SystemTraits.h>
#include <Animacore/SystemInterface.h>

namespace core
{
	struct CharacterController;
}

namespace core
{
	struct OnStartSystem;
	struct OnFinishSystem;
	class PhysicsScene;
	struct Animator;
	struct Sound;
}

namespace mc
{
	struct OnInputLock;
	struct OnOpenSettings;
	struct OnCloseSettings;

	class CharacterMovementSystem : public core::ISystem, public core::IUpdateSystem, public core::IPreUpdateSystem, public core::IFixedSystem
	{
	public:
		CharacterMovementSystem(core::Scene& scene);
		~CharacterMovementSystem();

		void PreUpdate(core::Scene& scene, float tick) override;
		void operator()(core::Scene& scene, float tick) override;
		void operator()(core::Scene& scene) override;

	private:
		void cameraLock(const mc::OnInputLock& event);
		void startSystem(const core::OnStartSystem& event);
		void finishSystem(const core::OnFinishSystem& event);
		void openSettings(const OnOpenSettings& event);
		void closeSettings(const OnCloseSettings& event);
		void playFootStepSound(core::Scene& scene);

	public:

	private:
		std::shared_ptr<core::PhysicsScene> _physicsScene = nullptr;
		entt::dispatcher* _dispatcher = nullptr;

		bool _isCameraLocked = false;
		bool _isMovementLocked = false;
		entt::entity _camera = entt::null;
		entt::entity _hand = entt::null;
		entt::entity _player = entt::null;
		core::Animator* _animator = nullptr;

		constexpr static float _playSoundThreshold = 0.3f;
		float _playSoundTImeAccum = 0.f;

		bool _isSettingsOpen = false;

		entt::entity _walkGroundSoundEntity = entt::null;
		core::Sound* _walkGroundSound = nullptr;
		entt::entity _walkWoodenSoundEntity = entt::null;
		core::Sound* _walkWoodenSound = nullptr;
		entt::entity _walkTruckSoundEntity = entt::null;
		core::Sound* _walkTruckSound = nullptr;
		entt::entity _walkTatamiSoundEntity = entt::null;
		core::Sound* _walkTatamiSound = nullptr;

		core::Sound* _jumpSound = nullptr;

		bool _skipFrameOnce = true;

		entt::entity _gameManager = entt::null;
	};
}
DEFINE_SYSTEM_TRAITS(mc::CharacterMovementSystem)


