// launcher.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//
#include "pch.h"
#include "LauncherProcess.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	core::ProcessInfo info;
	info.hInstance = hInstance;
	info.title = L"Midnight Clean-up";
	info.width = 0;
	info.height = 0;
	info.api = Renderer::API::NONE;
	info.enableImGui = false;
	info.proc = WndProc;
	info.windowStyle = WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX;
	
	//info.startScene = "";

	launcher::LauncherProcess process(info);

	process.Loop();

    return 0;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
    case WM_MOUSEWHEEL:
		launcher::LauncherProcess::mouseWheel = GET_WHEEL_DELTA_WPARAM(wParam);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}