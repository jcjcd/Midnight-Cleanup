#pragma once
#include "../Animacore/Entity.h"

class Renderer;

namespace core
{
	class Scene;
}

namespace mc
{
	struct OnInputLock
	{
		bool isCameraLock = false;
		bool isMovementLock = false;

		OnInputLock(bool isCameraLock, bool isMovementLock) :
			isCameraLock(isCameraLock),
			isMovementLock(isMovementLock) {}
	};

	struct OnProcessFurniture
	{
		core::Scene* scene;

		OnProcessFurniture(core::Scene& scene) : scene(&scene) {}
	};

	struct OnProcessWipe
	{
		core::Scene* scene;

		OnProcessWipe(core::Scene& scene) : scene(&scene) {}
	};

	struct OnOpenDoor
	{
		core::Scene* scene;
		entt::entity entity;

		OnOpenDoor(core::Scene& scene, entt::entity entity) : scene(&scene), entity(entity) {}
	};

	struct OnDiscardTool
	{
		core::Scene* scene;

		OnDiscardTool(core::Scene& scene) : scene(&scene) {}
	};

	struct OnSwitchPushed
	{
		core::Scene* scene;
		entt::entity entity;

		OnSwitchPushed(core::Scene& scene, entt::entity entity) : scene(&scene), entity(entity) {}
	};

	struct OnHandleFlashLight
	{
		core::Scene* scene;
		entt::entity entity;
		bool isDiscarding;

		OnHandleFlashLight(core::Scene& scene, entt::entity entity, bool isDiscarding) : scene(&scene), entity(entity), isDiscarding(isDiscarding) {}
	};

	struct OnGameOver
	{
		core::Scene* scene;

		OnGameOver(core::Scene& scene) : scene(&scene) {}
	};

	struct OnApplyChange { };

	struct OnDiscardChange { };

	struct OnOpenSettings
	{
		core::Scene* scene;

		OnOpenSettings(core::Scene& scene) : scene(&scene) {}
	};

	struct OnCloseSettings
	{
		core::Scene* scene;

		OnCloseSettings(core::Scene& scene) : scene(&scene) {}
	};

	struct OnDecreaseStain
	{
		core::Scene* scene;
		entt::entity entity;

		OnDecreaseStain(core::Scene& scene, entt::entity entity) : scene(&scene), entity(entity) {}
	};

	struct OnIncreaseStain
	{
		core::Scene* scene;
		entt::entity entity;

		OnIncreaseStain(core::Scene& scene, entt::entity entity) : scene(&scene), entity(entity) {}
	};

	struct OnShowCaption
	{
		core::Scene* scene;
		uint32_t index;
		float time;

		OnShowCaption(core::Scene& s, uint32_t c, float t) : scene(&s), index(c), time(t) {}
	};

	struct OnTriggerGhostEvent
	{
		core::Scene* scene;
		uint32_t room;

		OnTriggerGhostEvent(core::Scene& s, uint32_t r) : scene(&s), room(r) {}
	};

	struct OnNotifyToolChanged
	{
		core::Scene* scene;
		entt::entity entity;

		OnNotifyToolChanged(core::Scene& s, entt::entity e) : scene(&s), entity(e) {}
	};
};