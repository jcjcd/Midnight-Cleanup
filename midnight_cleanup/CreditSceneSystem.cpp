#include "pch.h"
#include "CreditSceneSystem.h"
#include "McTagsAndLayers.h"

#include <Animacore/Scene.h>
#include <Animacore/RenderComponents.h>
#include <Animacore/CoreTagsAndLayers.h>

#include "Animacore/CoreComponents.h"


mc::CreditSceneSystem::CreditSceneSystem(core::Scene& scene)
	: ISystem(scene)
{
	_dispatcher = scene.GetDispatcher();
	_dispatcher->sink<core::OnStartSystem>().connect<&CreditSceneSystem::startSystem>(this);
}

mc::CreditSceneSystem::~CreditSceneSystem()
{
	_dispatcher->disconnect(this);
}

void mc::CreditSceneSystem::operator()(core::Scene& scene)
{
	auto& registry = *scene.GetRegistry();
	auto& input = scene.GetRegistry()->ctx().get<core::Input>();

	for (auto&& [entity, tag] : registry.view<core::Tag>().each())
	{
		if (tag.id == tag::Credit::id)
		{
			auto& ui2D = registry.get<core::UI2D>(entity);

			_timeElapsed += FIXED_TIME_STEP;

			if (input.keyStates[VK_LBUTTON] == core::Input::State::Hold)
				_timeElapsed += FIXED_TIME_STEP * 3;
			else if (input.keyStates[VK_RBUTTON] == core::Input::State::Hold)
				_timeElapsed += FIXED_TIME_STEP * -3;
			

			if (_timeElapsed <= 30.f)
				ui2D.position.y = std::lerp(_startPositionY, 1580.0f, _timeElapsed / 30.0f);

			if (_timeElapsed >= 40.f)
				scene.GetDispatcher()->enqueue<core::OnChangeScene>("./Resources/Scenes/title.scene");
		}
	}
}

void mc::CreditSceneSystem::startSystem(const core::OnStartSystem& event)
{
	for (auto&& [entity, tag] : event.scene->GetRegistry()->view<core::Tag>().each())
	{
		if (tag.id == tag::Credit::id)
			_startPositionY = event.scene->GetRegistry()->get<core::UI2D>(entity).position.y;
	}
}
