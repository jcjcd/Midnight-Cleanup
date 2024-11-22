#pragma once
#include "McComponents.h"
#include "Animacore/SystemInterface.h"
#include "Animacore/SystemTraits.h"

namespace core
{
	struct OnStartSystem;
	struct OnFinishSystem;
}

namespace mc
{
	struct OnGameOver;
	struct OnCheckAbandoned;
	struct OnTriggerGhostEvent;

	class TimeLimitSystem : public core::ISystem, public core::IUpdateSystem, public core::IFixedSystem
	{
	public:
		TimeLimitSystem(core::Scene& scene);
		~TimeLimitSystem();

		void operator()(core::Scene& scene, float tick) override;
		void operator()(core::Scene& scene) override;

	private:
		void startSystem(const core::OnStartSystem& event);
		void finishSystem(const core::OnFinishSystem& event);
		void gameOver(const OnGameOver& event);
		void triggerGhostEvent(const OnTriggerGhostEvent& event);
		void openDoor(entt::registry& registry, float animationTime);
		void closeDoor(entt::registry& registry, float animationTime);

	private:
		entt::dispatcher* _dispatcher = nullptr;
		float _timeLimit = 600.0f;

		entt::entity _gameManager = entt::null;
		entt::entity _tv = entt::null;

		float _blinkTimeAccum = 0.0f;
		bool _isBlinkOn = false;
		constexpr static float _blinkInterval = 0.5f;

		float _closeTimeElapsed = 0.0f;
		float _openTimeElapsed = 0.0f;
		entt::entity _endingCamera = entt::null;
		entt::entity _truckDoorL = entt::null;
		entt::entity _truckDoorR = entt::null;
		entt::entity _truckPedestal = entt::null;
		bool _isTriggered = false;
		bool _isDoorOpen = false;
		bool _isPedestralOpen = true;

		bool _isGhostEventTriggered[Room::End] = { false };

		static constexpr float _defaultYAngle[2]
		{
			-148.0f,
			-29.0f
		};

		float _defaultPosittionZ = 0.0f;
		Quaternion _defaultRotation = Quaternion::Identity;
	};
}

DEFINE_SYSTEM_TRAITS(mc::TimeLimitSystem)