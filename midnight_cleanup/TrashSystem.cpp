#include "pch.h"
#include "TrashSystem.h"

#include "Animacore/Scene.h"
#include <Animacore/CoreComponents.h>

#include "McComponents.h"
#include "McTagsAndLayers.h"

mc::TrashSystem::TrashSystem(core::Scene& scene)
	: ISystem(scene)
	, _dispatcher(scene.GetDispatcher())
{
	_dispatcher->sink<core::OnStartSystem>()
		.connect<&TrashSystem::startSystem>(this);

	_dispatcher->sink<core::OnFinishSystem>()
		.connect<&TrashSystem::finishSystem>(this);
}

mc::TrashSystem::~TrashSystem()
{
	_dispatcher->disconnect(this);
}

void mc::TrashSystem::operator()(core::Scene& scene, float tick)
{
	if(!scene.IsPlaying())
		return;

	auto registry = scene.GetRegistry();
	auto view = registry->view<DestroyableTrash>();

	for(auto&& [entity, trash] : view.each())
	{
		if (trash.isGrabbed)
			continue;

		if (trash.inTrashBox)
		{
			trash.remainTime -= tick;

			if (trash.remainTime < 0.f)
			{
				scene.DestroyEntity(entity);
				if (_quest)
					_quest->cleanedTrashes += 1;
			}
		}
	}
}

void mc::TrashSystem::OnCollisionEnter(entt::entity self, entt::entity other, entt::registry& registry)
{
	if (auto trash = registry.try_get<DestroyableTrash>(other))
	{
		trash->inTrashBox = true;
	}
}

void mc::TrashSystem::OnCollisionStay(entt::entity self, entt::entity other, entt::registry& registry)
{

}

void mc::TrashSystem::OnCollisionExit(entt::entity self, entt::entity other, entt::registry& registry)
{
	if (auto trash = registry.try_get<DestroyableTrash>(other))
	{
		trash->inTrashBox = false;
		trash->remainTime = trash->destroyTime;
	}
}

void mc::TrashSystem::startSystem(const core::OnStartSystem& event)
{
	_dispatcher->trigger<core::OnRegisterCollisionHandler>({ tag::TrashBox::id, this });

	if (event.scene->GetRegistry()->ctx().contains<mc::Quest>())
		_quest = &event.scene->GetRegistry()->ctx().get<mc::Quest>();
	else
		_quest = &event.scene->GetRegistry()->ctx().emplace<mc::Quest>();
}

void mc::TrashSystem::finishSystem(const core::OnFinishSystem& event)
{
	_dispatcher->trigger<core::OnRemoveCollisionHandler>({ tag::TrashBox::id, this });

	_quest = nullptr;
}
