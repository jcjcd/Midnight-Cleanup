// Animatool.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//
#include "pch.h"

#include "ToolProcess.h"


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	core::ProcessInfo info;
	info.hInstance = hInstance;
	info.title = L"Animatool";
	info.width = 0;
	info.height = 0;
	info.api = Renderer::API::NONE;
	info.enableImGui = true;
	info.proc = WndProc;

	tool::ToolProcess process(info);

	process.Loop();

	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return 0;

	switch (message)
	{
		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED)
			{
				tool::ToolProcess::isResized = true;
				tool::ToolProcess::isMinimized = false;
			}
			else
				tool::ToolProcess::isMinimized = true;
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_MOUSEWHEEL:
			tool::ToolProcess::mouseWheel = GET_WHEEL_DELTA_WPARAM(wParam);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}