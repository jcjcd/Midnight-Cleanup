#include "pch.h"
#include "IngameLightSystem.h"

#include "McComponents.h"
#include "McTagsAndLayers.h"

#include <Animacore/Scene.h>
#include <Animacore/CoreComponents.h>
#include <Animacore/RenderComponents.h>

#include "McEvents.h"

mc::IngameLightSystem::IngameLightSystem(core::Scene& scene)
	: core::ISystem(scene), _dispatcher(scene.GetDispatcher())
{
	_dispatcher->sink<mc::OnSwitchPushed>().connect<&IngameLightSystem::switchPushed>(this);
	_dispatcher->sink<mc::OnHandleFlashLight>().connect<&IngameLightSystem::handleFlashlight>(this);
}

mc::IngameLightSystem::~IngameLightSystem()
{
	_dispatcher->disconnect(this);
}

void mc::IngameLightSystem::operator()(core::Scene& scene, Renderer& renderer, float tick)
{
	if (!scene.IsPlaying())
		return;

	auto& registry = *scene.GetRegistry();

	for (auto&& [entity, meshRenderer, relationship] : registry.view<core::MeshRenderer, core::Relationship>().each())
	{
		for (auto& child : relationship.children)
		{
			if (core::LightCommon* common = registry.try_get<core::LightCommon>(child))
			{
				if (!common->isOn)
					meshRenderer.emissiveFactor = 0.0f;
				else
					meshRenderer.emissiveFactor = 1.0f;
			}
		}
	}

	for (auto&& [entity, curSwitch] : registry.view<mc::Switch>().each())
	{
		if (curSwitch.isOn)
		{
			bool isLightOff = false;
			core::LightCommon* light0 = registry.try_get<core::LightCommon>(curSwitch.lights.light0);
			core::LightCommon* light1 = registry.try_get<core::LightCommon>(curSwitch.lights.light1);
			core::LightCommon* light2 = registry.try_get<core::LightCommon>(curSwitch.lights.light2);

			if (light0 && !light0->isOn)
				isLightOff = true;

			if (light1 && !light1->isOn)
				isLightOff = true;

			if (light2 && !light2->isOn)
				isLightOff = true;

			if (isLightOff)
			{
				curSwitch.isOn = false;
				registry.patch<mc::Switch>(entity);

				if (light0)
				{
					light0->isOn = false;
					registry.patch<core::LightCommon>(curSwitch.lights.light0);
				}

				if (light1)
				{
					light1->isOn = false;
					registry.patch<core::LightCommon>(curSwitch.lights.light1);
				}

				if (light2)
				{
					light2->isOn = false;
					registry.patch<core::LightCommon>(curSwitch.lights.light2);
				}
			}
		}
	}
}

void mc::IngameLightSystem::switchPushed(const mc::OnSwitchPushed& event)
{
	auto& registry = *event.scene->GetRegistry();
	mc::Switch& curSwitch = registry.get<mc::Switch>(event.entity);
	auto& sound = registry.get<core::Sound>(event.entity);

	curSwitch.isOn = !curSwitch.isOn;
	sound.isPlaying = true;
	registry.patch<mc::Switch>(event.entity);
	registry.patch<core::Sound>(event.entity);

	if (curSwitch.lights.light0 != entt::null)
	{
		auto& light = registry.get<core::LightCommon>(curSwitch.lights.light0);
		light.isOn = !light.isOn;
		registry.patch<core::LightCommon>(curSwitch.lights.light0);
	}

	if (curSwitch.lights.light1 != entt::null)
	{
		auto& light = registry.get<core::LightCommon>(curSwitch.lights.light1);
		light.isOn = !light.isOn;
		registry.patch<core::LightCommon>(curSwitch.lights.light1);
	}

	if (curSwitch.lights.light2 != entt::null)
	{
		auto& light = registry.get<core::LightCommon>(curSwitch.lights.light2);
		light.isOn = !light.isOn;
		registry.patch<core::LightCommon>(curSwitch.lights.light2);
	}

	core::WorldTransform& transform = registry.get<core::WorldTransform>(curSwitch.lever);

	float radian = DirectX::XMConvertToRadians(DirectX::XM_2PI * 8);

	// 기본이 꺼져있는 상태, 켜지면 레버가 왼쪽으로 90도 회전
	if (curSwitch.isOn)
		transform.rotation *= Quaternion::CreateFromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), radian);
	else
		transform.rotation *= Quaternion::CreateFromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), -radian);

	registry.patch<core::WorldTransform>(curSwitch.lever);
}

void mc::IngameLightSystem::handleFlashlight(const mc::OnHandleFlashLight& event)
{
	auto& registry = *event.scene->GetRegistry();
	auto& flashlight = registry.get<mc::Flashlight>(event.entity);
	auto& light = registry.get<core::LightCommon>(event.entity);

	if (!event.isDiscarding)
	{
		flashlight.isOn = !flashlight.isOn;
		light.isOn = !light.isOn;
	}
	else
	{
		flashlight.isOn = false;
		light.isOn = false;
	}

	registry.patch<mc::Flashlight>(event.entity);
	registry.patch<core::LightCommon>(event.entity);
}
