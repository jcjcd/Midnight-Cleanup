// ChangDX.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "framework.h"
#include "ChangDX.h"
#include <crtdbg.h>  
#include <objbase.h>
#include "C3DApp.h"

#include <unordered_map>

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 613; }

extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	// 지정된 힙에 대한 기능을 사용하도록 설정합니다.
	// 손상에 대한 종료 기능을 사용하도록 설정합니다. 힙 관리자가 프로세스에서 사용하는 모든 힙에서 오류를 감지하면 Windows 오류 보고 서비스를 호출하고 프로세스를 종료합니다. 
	// by msdn
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	//_CrtSetBreakAlloc(131574);

	if (SUCCEEDED(CoInitialize(NULL)))
	{
		{
			C3DApp* app = nullptr;
			app = new C3DApp;

			if (SUCCEEDED(app->Initialize()))
			{
				app->RunMessageLoop();
			}

			app->Finalize();
			delete app;
		}
		CoUninitialize();
	}
#ifdef _DEBUG
	_ASSERT(_CrtCheckMemory());
#endif


	return 0;

}

