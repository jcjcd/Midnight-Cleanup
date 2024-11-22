#include "pch.h"
#include "CoreProcess.h"

#include "MetaFuncs.h"

core::CoreProcess::CoreProcess(ProcessInfo info)
	: _processInfo(std::move(info))
{
	loadConfigData();

	WNDCLASSEXW wcex{};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = _processInfo.proc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = _processInfo.hInstance;
	wcex.hIcon = nullptr;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = nullptr;
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = L"Anim_all Engine";
	wcex.hIconSm = nullptr;

	RegisterClassExW(&wcex);

	//// 리사이즈 불가능하게 

	
	RECT rect{0, 0, (LONG)_processInfo.width, (LONG)_processInfo.height };
	AdjustWindowRect(&rect, _processInfo.windowStyle, false);

	_hWnd = CreateWindowW(
		wcex.lpszClassName,
		_processInfo.title.c_str(),
		_processInfo.windowStyle,
		CW_USEDEFAULT,
		0,
		rect.right - rect.left,
		rect.bottom - rect.top,
		nullptr,
		nullptr,
		_processInfo.hInstance,
		nullptr
	);

	CenterWindow();

	ShowWindow(_hWnd, SW_SHOWNORMAL);
	UpdateWindow(_hWnd);

	_renderer = Renderer::Create(_hWnd, _processInfo.width, _processInfo.height, _processInfo.api, false);
	_renderer->SetFullScreen(_processInfo.fullScreen);

	RegisterCoreMetaData();
}

core::CoreProcess::~CoreProcess()
{
	saveConfigData();
}

void core::CoreProcess::CenterWindow()
{
	// 화면의 작업 영역 얻기
	RECT rectWindow;
	GetWindowRect(_hWnd, &rectWindow);

	// 현재 모니터의 정보 얻기
	HMONITOR hMonitor = MonitorFromWindow(_hWnd, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO monitorInfo;
	monitorInfo.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(hMonitor, &monitorInfo);
	RECT rectScreen = monitorInfo.rcWork; // 작업 영역 (작업 표시줄 제외)

	// 중앙 좌표 계산
	int windowWidth = rectWindow.right - rectWindow.left;
	int windowHeight = rectWindow.bottom - rectWindow.top;
	int posX = rectScreen.left + (rectScreen.right - rectScreen.left - windowWidth) / 2;
	int posY = rectScreen.top + (rectScreen.bottom - rectScreen.top - windowHeight) / 2;

	// 윈도우 위치 설정
	SetWindowPos(_hWnd, HWND_TOP, posX, posY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

void core::CoreProcess::loadConfigData()
{
	const wchar_t* configpath = _processInfo.enableImGui ? TOOL_CONFIG_FILE : CONFIG_FILE;
	LPTSTR str = new TCHAR[256];
	GetPrivateProfileString(L"Graphics", L"ScreenWidth", nullptr, str, 256, configpath);
	lstrlen(str) <= 0 ? (_processInfo.width = 1920) : (_processInfo.width = _wtoi(str));
	GetPrivateProfileString(L"Graphics", L"ScreenHeight", nullptr, str, 256, configpath);
	lstrlen(str) <= 0 ? (_processInfo.height = 1080) : (_processInfo.height = _wtoi(str));
	GetPrivateProfileString(L"Graphics", L"GraphicAPI", nullptr, str, 256, configpath);
	if (lstrlen(str) <= 0) {
		_processInfo.api = Renderer::API::DirectX11;
	}
	else {
		if (wcscmp(str, L"DirectX11") == 0) {
			_processInfo.api = Renderer::API::DirectX11;
		}
		else if (wcscmp(str, L"DirectX12") == 0) {
			assert(false && "DirectX12 is not supported yet");
			_processInfo.api = Renderer::API::DirectX11;
		}
		else {
			_processInfo.api = Renderer::API::DirectX11;
		}
	}
	GetPrivateProfileString(L"Graphics", L"FullScreen", nullptr, str, 256, configpath);
	lstrlen(str) <= 0 ? (_processInfo.fullScreen = false) : (_processInfo.fullScreen = _wtoi(str));
	GetPrivateProfileInt(L"Graphics", L"WindowedFullScreen", 0, configpath) ? _processInfo.windowedFullScreen = true : _processInfo.windowedFullScreen = false;
	delete[] str;

}

void core::CoreProcess::saveConfigData()
{
	const wchar_t* configpath = _processInfo.enableImGui ? TOOL_CONFIG_FILE : CONFIG_FILE;
	WritePrivateProfileString(L"Graphics", L"ScreenWidth", std::to_wstring(_processInfo.width).c_str(), configpath);
	WritePrivateProfileString(L"Graphics", L"ScreenHeight", std::to_wstring(_processInfo.height).c_str(), configpath);
	WritePrivateProfileString(L"Graphics", L"GraphicAPI", _processInfo.api == Renderer::API::DirectX11 ? L"DirectX11" : L"DirectX12", configpath);
	WritePrivateProfileString(L"Graphics", L"FullScreen", std::to_wstring(_processInfo.fullScreen).c_str(), configpath);

}
