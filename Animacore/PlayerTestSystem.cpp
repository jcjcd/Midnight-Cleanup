#include "pch.h"
#include "PlayerTestSystem.h"
#include "CoreComponents.h"
#include "RenderComponents.h"
#include "Scene.h"

void core::PlayerTestSystem::operator()(Scene& scene, float tick)
{
	auto& registry = *scene.GetRegistry();
	auto& kbd = registry.ctx().get<Input>();
	auto view = registry.view<Animator>();

	static float speed = 0.0f;

	for (auto&& [entity, animator] : view.each())
	{
		auto k = KEY('a');
		auto state = kbd.keyStates[k];
		if (kbd.keyStates[VK_RIGHT] == Input::State::Hold)
		{
			speed += tick;
			animator.parameters["Speed"].value = speed;
		}
		else if (kbd.keyStates[VK_LEFT] == Input::State::Hold)
		{
			speed -= tick;
			animator.parameters["Speed"].value = speed;
		}

		if (kbd.keyStates[VK_SPACE] == Input::State::Down)
		{
			animator.parameters["IsGround"].value = false;
		}
		if (kbd.keyStates[VK_SPACE] == Input::State::Up)
		{
			animator.parameters["IsGround"].value = true;
		}

	}
}
