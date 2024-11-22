#include "pch.h"
#include "DropSoundSystem.h"

#include "McTagsAndLayers.h"
#include "Animacore/CoreComponents.h"
#include "Animacore/Scene.h"

mc::DropSoundSystem::DropSoundSystem(core::Scene& scene)
	: ISystem(scene)
	, _dispatcher(scene.GetDispatcher())
{
	_dispatcher->sink<core::OnStartSystem>()
		.connect<&DropSoundSystem::startSystem>(this);

	_dispatcher->sink<core::OnFinishSystem>()
		.connect<&DropSoundSystem::finishSystem>(this);
}

mc::DropSoundSystem::~DropSoundSystem()
{
	_dispatcher->disconnect(this);
}

void mc::DropSoundSystem::OnCollisionEnter(entt::entity self, entt::entity other, entt::registry& registry)
{
	if (auto sound = registry.try_get<core::Sound>(self))
	{
		if (sound->isPlaying)
			return;

		sound->isPlaying = true;
		registry.patch<core::Sound>(self);
	}
}

void mc::DropSoundSystem::OnCollisionStay(entt::entity self, entt::entity other, entt::registry& registry)
{

}

void mc::DropSoundSystem::OnCollisionExit(entt::entity self, entt::entity other, entt::registry& registry)
{

}

void mc::DropSoundSystem::startSystem(const core::OnStartSystem& event)
{
	_dispatcher->trigger<core::OnRegisterCollisionHandler>({ tag::Trash::id, this });
	_dispatcher->trigger<core::OnRegisterCollisionHandler>({ tag::Mess::id, this });
	_dispatcher->trigger<core::OnRegisterCollisionHandler>({ tag::Tool::id, this });
}

void mc::DropSoundSystem::finishSystem(const core::OnFinishSystem& event)
{
	_dispatcher->trigger<core::OnRemoveCollisionHandler>({ tag::Trash::id, this });
	_dispatcher->trigger<core::OnRemoveCollisionHandler>({ tag::Mess::id, this });
	_dispatcher->trigger<core::OnRemoveCollisionHandler>({ tag::Tool::id, this });
}
