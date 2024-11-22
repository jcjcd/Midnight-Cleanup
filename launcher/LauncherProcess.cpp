#include "pch.h"
#include "LauncherProcess.h"

#include <Animacore/Timer.h>
#include <Animacore/AnimatorSystem.h>

#include "Animacore/CoreSystemEvents.h"
#include "Animacore/Scene.h"

#include "Animacore/CoreComponents.h"

launcher::LauncherProcess::LauncherProcess(const core::ProcessInfo& info)
	: McProcess(info)
{
	if (_renderer->IsRayTracing())
		_renderer->InitializeTopLevelAS();

	_renderer->LoadMeshesFromDrive("./Resources/Models");
	_renderer->LoadMaterialsFromDrive("./Resources");
	_renderer->LoadShadersFromDrive("./Shaders");
	_renderer->LoadAnimationClipsFromDrive("./Resources/Animations");
	_renderer->LoadUITexturesFromDrive("./Resources/UI");
	_renderer->LoadParticleTexturesFromDrive("./Resources/Particles");
	_renderer->LoadFontsFromDrive("./Resources/Fonts");

	core::AnimatorSystem::controllerManager.LoadControllersFromDrive("./Resources", _renderer.get());

	_currentScene = std::make_shared<core::Scene>();
	_currentScene->LoadScene("./Resources/Scenes/title.scene");

	// configuration을 가져온다
	auto& config = _currentScene->GetRegistry()->ctx().get<core::Configuration>();
	config.width = _processInfo.width;
	config.height = _processInfo.height;

	_currentScene->Start(_renderer.get());
	_currentScene->GetDispatcher()->sink<core::OnChangeScene>().connect<&LauncherProcess::changeScene>(this);
	_currentScene->GetDispatcher()->sink<core::OnChangeResolution>().connect<&LauncherProcess::changeResolution>(this);



	core::OnChangeResolution event;
	event.width = config.width;
	event.height = config.height;
	event.isFullScreen = _processInfo.fullScreen;
	event.isWindowedFullScreen = _processInfo.windowedFullScreen;

	changeResolution(event);
}

void launcher::LauncherProcess::Loop()
{
	// todo
	static core::Timer timer;

	MSG msg;

	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			timer.Update();
			const auto tick = timer.GetTick();

			// 씬 재생
			_currentScene->Update(tick);

			// 렌더링
			if (!LauncherProcess::isMinimized)
			{
				_renderer->BeginRender();

				// 씬 렌더
				_currentScene->Render(tick, _renderer.get());

				_renderer->EndRender();
			}

			_currentScene->ProcessEvent();
			mouseWheel = 0;

			changeExecution();
		}
	}
}

void launcher::LauncherProcess::changeScene(const core::OnChangeScene& event)
{
	_receiveChangeEvent = true;
	_scenePath = event.path;
}

void launcher::LauncherProcess::changeExecution()
{
	if (_receiveChangeEvent)
	{
		_currentScene->Finish(_renderer.get());
		_currentScene->LoadScene(_scenePath);
		_currentScene->Start(_renderer.get());
		//_titleBarHeight = GetSystemMetrics(SM_CYCAPTION);
		//_currentScene->GetDispatcher()->enqueue<core::OnGetTitleBarHeight>({ _titleBarHeight });

		_receiveChangeEvent = false;
		_scenePath.clear();
	}
}

void launcher::LauncherProcess::changeResolution(const core::OnChangeResolution& event)
{
	if (event.isFullScreen)
	{
		_renderer->SetFullScreen(true);
	}
	else
	{
		if (_renderer->IsFullScreen())
			_renderer->SetFullScreen(false);

		if (event.isWindowedFullScreen)
		{
			int width = GetSystemMetrics(SM_CXSCREEN);
			int height = GetSystemMetrics(SM_CYSCREEN);

			// 윈도우 크기를 해당 해상도로 변경
			SetWindowPos(_hWnd, HWND_TOP, 0, 0, width, height, SWP_SHOWWINDOW);
			_renderer->Resize(width, height);

			// 윈도우 스타일을 팝업으로 변경
			SetWindowLong(_hWnd, GWL_STYLE, WS_POPUP);
			ShowWindow(_hWnd, SW_MAXIMIZE);
			//_titleBarHeight = 0;
		}
		else
		{
			if (event.width > 0 && event.height > 0)
			{
				// 윈도우 크기를 해당 해상도로 변경

				// 창 크기를 계산할 RECT 구조체를 정의합니다.
				RECT rect = { 0, 0,  event.width, event.height };
				// 윈도우 크기를 계산합니다.
				AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX, false);

				// 윈도우 크기를 변경합니다.
				int width = rect.right - rect.left;
				int height = rect.bottom - rect.top;

				// 현재 윈도우 위치를 가져온다.
				RECT windowRect;
				GetWindowRect(_hWnd, &windowRect);

				SetWindowPos(_hWnd, HWND_TOP, windowRect.left, windowRect.top, width, height, SWP_SHOWWINDOW);
				_renderer->Resize(event.width, event.height);

				SetWindowLong(_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX);
				ShowWindow(_hWnd, SW_SHOWNORMAL);
				//_titleBarHeight = GetSystemMetrics(SM_CYCAPTION);
			}
		}
	}

	// 윈도우 api를 통해 실제 해상도를 가져온다.
	// 스타일을 통해 진짜 클라이언트 사이즈를 가져온다.
	RECT rect;
	GetClientRect(_hWnd, &rect);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	// configuration을 가져온다
	auto& config = _currentScene->GetRegistry()->ctx().get<core::Configuration>();
	config.width = width;
	config.height = height;

	_currentScene->GetDispatcher()->trigger<core::OnDestroyRenderResources>({ *_currentScene, _renderer.get() });
	_currentScene->GetDispatcher()->trigger<core::OnCreateRenderResources>({ *_currentScene, _renderer.get() });
	_currentScene->GetDispatcher()->trigger<core::OnLateCreateRenderResources>({ *_currentScene, _renderer.get() });
}

