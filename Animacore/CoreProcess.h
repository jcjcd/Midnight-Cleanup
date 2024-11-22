#pragma once

#include "../Animavision/Renderer.h"

#include <Windows.h>

class Renderer;

namespace core
{
	LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	struct ProcessInfo
	{
		HINSTANCE hInstance = nullptr;
		std::wstring title = L"Empty";
		uint32_t width = 0;
		uint32_t height = 0;
		bool fullScreen = false;
		bool windowedFullScreen = false;
		Renderer::API api = Renderer::API::NONE;
		bool enableImGui = false;
		WNDPROC proc = nullptr;
		uint32_t windowStyle = WS_OVERLAPPEDWINDOW;
	};

	class CoreProcess
	{ 
	public:
		CoreProcess(ProcessInfo info);
		virtual ~CoreProcess();

		virtual void Loop() = 0;
		void CenterWindow();

		inline static int mouseWheel = 0;

	private:
		static constexpr wchar_t CONFIG_FILE[] = L"./config.ini";
		static constexpr wchar_t TOOL_CONFIG_FILE[] = L"./tool_config.ini";

		void loadConfigData();
		void saveConfigData();

	protected:
		ProcessInfo _processInfo;
		HWND _hWnd = nullptr;

		std::unique_ptr<Renderer> _renderer;
	};
}

