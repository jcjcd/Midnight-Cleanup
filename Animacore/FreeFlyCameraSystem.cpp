#include "pch.h"
#include "FreeFlyCameraSystem.h"

#include "Scene.h"

#include "RenderComponents.h"
#include "CoreComponents.h"



core::FreeFlyCameraSystem::FreeFlyCameraSystem(Scene& scene) :
	ISystem(scene)
{
	_dispatcher = scene.GetDispatcher();
}

void core::FreeFlyCameraSystem::operator()(Scene& scene, float tick)
{
	if (!scene.IsPlaying())
		return;

	auto& registry = *scene.GetRegistry();
	auto& kbd = registry.ctx().get<Input>();

	for (auto&& [entity, camera, freefly] : registry.view<core::Camera, core::FreeFly>().each())
	{
		auto& transform = registry.get<core::WorldTransform>(entity);

		Vector3 move = Vector3::Zero;
		if (kbd.keyStates['W'] == Input::State::Hold)
		{
			move -= transform.matrix.Forward();
		}
		if (kbd.keyStates['S'] == Input::State::Hold)
		{
			move += transform.matrix.Forward();
		}
		if (kbd.keyStates['A'] == Input::State::Hold)
		{
			move -= transform.matrix.Right();
		}
		if (kbd.keyStates['D'] == Input::State::Hold)
		{
			move += transform.matrix.Right();
		}
		if (kbd.keyStates['E'] == Input::State::Hold)
		{
			move += transform.matrix.Up();
		}
		if (kbd.keyStates['Q'] == Input::State::Hold)
		{
			move -= transform.matrix.Up();
		}

		move.Normalize();
		transform.position += move * freefly.moveSpeed * tick;


		Vector2 mousePos = kbd.mousePosition;
		Vector2 offset = mousePos - freefly.prevMousePos;
		freefly.prevMousePos = mousePos;

		if (offset.x > 0 || offset.x < 0 || offset.y < 0 || offset.y > 0)
		{
			//yaw
			Quaternion r2 = Quaternion::CreateFromAxisAngle({ 0,1,0 }, offset.x * freefly.rotateSpeed * tick);

			//pitch
			Quaternion r3 = Quaternion::CreateFromAxisAngle({ 1,0,0 }, offset.y * freefly.rotateSpeed * tick);

			Quaternion r4 = r3 * transform.rotation;

			//cameraOrientation * frameYaw
			Quaternion r5 = r4 * r2;

			transform.rotation = r5;
		}

		registry.patch<core::WorldTransform>(entity);
	}

}
