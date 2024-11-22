#pragma once
#include "McEvents.h"
#include "McComponents.h"
#include "Animacore/SystemInterface.h"
#include "Animacore/SystemTraits.h"

namespace mc
{
	struct Quest;
	struct OnOpenDoor;
	struct OnTriggerGhostEvent;

	class RoomEnterSystem : public core::ISystem, public core::IFixedSystem, public core::ICollisionHandler
	{
	public:
		explicit RoomEnterSystem(core::Scene& scene);
		~RoomEnterSystem() override;

		void operator()(core::Scene& scene) override;

		void OnCollisionEnter(entt::entity self, entt::entity other, entt::registry& registry) override;
		void OnCollisionStay(entt::entity self, entt::entity other, entt::registry& registry) override;
		void OnCollisionExit(entt::entity self, entt::entity other, entt::registry& registry) override;

	private:
		core::Scene* _scene = nullptr;
		entt::dispatcher* _dispatcher = nullptr;
		mc::Quest* _quest = nullptr;
		entt::entity _player = entt::null;

		mc::Room* _enteredRoom = nullptr;
		mc::Room* _playerRoom = nullptr;

		float _enterElapsedTime[Room::End] = { 0.f };
		bool _isTriggered[Room::End] = { false };

		void startSystem(const core::OnStartSystem& event);
		void finishSystem(const core::OnFinishSystem& event);

		void openDoor(const mc::OnOpenDoor& event);
		void triggerGhostEvent(const mc::OnTriggerGhostEvent& event);

		void outlineMesses(core::Scene& scene, Room* prevRoom, Room* nextRoom);

		entt::entity _triggerSound = entt::null;
	};
}
DEFINE_SYSTEM_TRAITS(mc::RoomEnterSystem)
