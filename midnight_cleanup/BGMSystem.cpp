#include "pch.h"
#include "BGMSystem.h"

#include "Animacore/Scene.h"
#include "Animacore/CoreComponents.h"

mc::BGMSystem::BGMSystem(core::Scene& scene) :
	core::ISystem(scene)
{
	_dispatcher = scene.GetDispatcher();

	_dispatcher->sink<core::OnStartSystem>().connect<&BGMSystem::startSystem>(this);
	_dispatcher->sink<core::OnFinishSystem>().connect<&BGMSystem::finishSystem>(this);
}

mc::BGMSystem::~BGMSystem()
{
	_dispatcher->disconnect(this);
}

void mc::BGMSystem::startSystem(const core::OnStartSystem& event)
{
	// registry
	auto* registry = event.scene->GetRegistry();
	auto&& bgm = registry->ctx().emplace<core::BGM>();

	auto& sound = bgm.sound;

	if (sound.isPlaying)
		return;

	sound.isLoop = true;
	sound.isPlaying = true;

	sound.path = "./Resources/Sound/ambient/BGM_cat.wav";

	// 디스패쳐에 이벤트 보냄
	_dispatcher->trigger<core::OnUpdateBGMState>({*event.scene});
}

void mc::BGMSystem::finishSystem(const core::OnFinishSystem& event)
{
}
