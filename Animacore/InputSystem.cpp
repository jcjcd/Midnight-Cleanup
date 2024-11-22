#include "pch.h"
#include "InputSystem.h"

#include "Scene.h"
#include "CoreProcess.h"
#include "CoreComponents.h"
#include "CoreSystemEvents.h"

core::InputSystem::InputSystem(Scene& scene)
	: ISystem(scene)
{
	_dispatcher = scene.GetDispatcher();
	_dispatcher->sink<OnStartSystem>().connect<&InputSystem::startSystem>(this);
	_dispatcher->sink<OnFinishSystem>().connect<&InputSystem::finishSystem>(this);
	_dispatcher->sink<OnChangeResolution>().connect<&InputSystem::changeResolution>(this);
}

void core::InputSystem::PreUpdate(Scene& scene, float tick)
{
	auto& input = scene.GetRegistry()->ctx().get<Input>();
	auto keySize = input.keyStates.size();

	if (const auto hWnd = GetFocus())
	{
		for (auto key = 0; key < keySize; ++key)
		{
			const bool isPushed = isKeyPressed(key);

			if (isPushed)
			{
				if (input._prevPushed[key])
					input.keyStates[key] = Input::State::Hold;
				else
					input.keyStates[key] = Input::State::Down;
			}
			else
			{
				if (input._prevPushed[key])
					input.keyStates[key] = Input::State::Up;
				else
					input.keyStates[key] = Input::State::None;
			}

			input._prevPushed[key] = isPushed;
		}

		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(hWnd, &pt);

		Vector2 currentMousePosition{ static_cast<float>(pt.x), static_cast<float>(pt.y) };
		currentMousePosition -= input.viewportPosition;

		// 뷰포트 좌표로 변환
		currentMousePosition.x /= input.viewportSize.x;
		currentMousePosition.y /= input.viewportSize.y;

		currentMousePosition.x *= static_cast<float>(_windowWidth);
		currentMousePosition.y *= static_cast<float>(_windowHeight);

		input.mousePosition = currentMousePosition;

		// Clamp일 경우
		if (global::mouseMode == global::MouseMode::Clamp)
		{
			if (currentMousePosition.x < 0)
				currentMousePosition.x = 0;
			else if (currentMousePosition.x > static_cast<float>(_windowWidth))
				currentMousePosition.x = static_cast<float>(_windowWidth);

			if (currentMousePosition.y < 0)
				currentMousePosition.y = 0;
			else if (currentMousePosition.y > static_cast<float>(_windowHeight))
				currentMousePosition.y = static_cast<float>(_windowHeight);
		}

		input.mouseDeltaPosition = currentMousePosition - input._prevMousePosition;
		input._prevMousePosition = currentMousePosition;

		// 화면 크기에 따라 마우스 이동량을 회전량으로 변환
		input.mouseDeltaRotation.x = input.mouseDeltaPosition.x / _windowWidth * 2.0f * DirectX::XM_PI;
		input.mouseDeltaRotation.y = input.mouseDeltaPosition.y / _windowHeight * 2.0f * DirectX::XM_PI;
		input.mouseWheel = CoreProcess::mouseWheel;

		if (global::mouseMode == global::MouseMode::Lock)
		{
			RECT rect;
			GetWindowRect(hWnd, &rect);

			// 클라이언트 좌표로 변환
			POINT centerPoint;
			centerPoint.x = (rect.left + rect.right) / 2;
			centerPoint.y = (rect.top + rect.bottom) / 2;

			// 스크린 좌표를 클라이언트 좌표로 변환
			ScreenToClient(hWnd, &centerPoint);

			Vector2 center{ static_cast<float>(centerPoint.x), static_cast<float>(centerPoint.y) };

			center -= input.viewportPosition;
			center.x /= input.viewportSize.x;
			center.y /= input.viewportSize.y;

			center.x *= _windowWidth;
			center.y *= _windowHeight;

			// 마우스 커서 이동
			ClientToScreen(hWnd, &centerPoint); // 클라이언트 좌표를 다시 스크린 좌표로 변환
			SetCursorPos(centerPoint.x, centerPoint.y);

			input._prevMousePosition = center;
		}
	}
	else
	{
		for (auto key = 0; key < keySize; ++key)
		{
			if (input._prevPushed[key])
				input.keyStates[key] = Input::State::Up;
			else
				input.keyStates[key] = Input::State::None;

			input._prevPushed[key] = false;
		}

		input.mousePosition = Vector2::Zero;
		input.mouseDeltaPosition = Vector2::Zero;
		input.mouseDeltaRotation = Vector2::Zero;
		input.mouseWheel = 0;
	}
}

bool core::InputSystem::isKeyPressed(int keyCode)
{
	if (GetAsyncKeyState(keyCode) & 0x8000)
		return true;

	return false;
}

void core::InputSystem::startSystem(const OnStartSystem& event)
{
	// Input 컴포넌트 삽입
	auto&& input = event.scene->GetRegistry()->ctx().emplace<Input>();

	// Configuration 참조
	auto& config = event.scene->GetRegistry()->ctx().get<Configuration>();

	if (config.width == 0 or config.height == 0)
	{
		_dispatcher->enqueue<OnThrow>(
			OnThrow::Warn,
			"[{}]\nMust set the window size in Window > Configuration.\nSet to the default value (1920X1080).",
			"Input System");

		_windowWidth = 1920;
		_windowHeight = 1080;
	}
	else
	{
		_windowWidth = config.width;
		_windowHeight = config.height;
	}

	_input = &input;

	input.viewportSize = { static_cast<float>(_windowWidth), static_cast<float>(_windowHeight) };
}

void core::InputSystem::finishSystem(const OnFinishSystem& event)
{
	event.scene->GetRegistry()->ctx().erase<Input>();

	_input = nullptr;

	_windowWidth = 0;
	_windowHeight = 0;
}

void core::InputSystem::changeResolution(const OnChangeResolution& event)
{
	_windowWidth = event.width;
	_windowHeight = event.height;

	if (_input)
		_input->viewportSize = { static_cast<float>(_windowWidth), static_cast<float>(_windowHeight) };

}
