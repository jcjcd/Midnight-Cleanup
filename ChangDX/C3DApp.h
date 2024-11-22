#pragma once

#include "resource.h"
#include "../Animavision/Renderer.h"
#include <memory>

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

#include "../Animavision/Material.h"

#include "StepTimer.h"

// Test TODO 지울것 
class Mesh;
class Shader;
class Model;
class Material;
class Texture;
class AnimatorController;

class C3DApp
{
public:
	C3DApp();
	~C3DApp();

	void CalculateFrameStats();
	void SetCustomWindowText(LPCWSTR text);
public:

	/// 용도: 창 클래스를 등록 한 후 인스턴스 핸들을 저장하고 주 창을 만듭니다.
	/// 주석:
	///      이 함수를 통해 핸들윈도우을 멤버변수에 저장하고 주 프로그램 창을 만든 다음 표시합니다.
	HRESULT Initialize();

	/// PeekMessage를 통해 윈도우의 메시지 루프를 처리한다.
	void RunMessageLoop();

	/// 데모 앱 종료시 해야 할 것들을 모아놓는다.
	void Finalize();

	/// 함수: WndProc(HWND, UINT, WPARAM, LPARAM)
	/// 용도: 주 창의 메시지를 처리합니다.
	static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

private:
	HWND m_hWnd;
	// 화면 크기
	int m_ScreenWidth;
	int m_ScreenHeight;

	StepTimer m_Timer;
	std::wstring m_Title;

	inline static bool m_bSizechanged = false;
	// 렌더러
	Renderer::API m_API = Renderer::API::NONE;
	std::unique_ptr<Renderer> m_Renderer;
	MaterialLibrary m_MaterialLibrary;

	std::vector<std::shared_ptr<Material>> m_Materials1;
	std::vector<std::shared_ptr<Material>> m_Materials2;
	std::vector<std::shared_ptr<Material>> m_Materials3;

	std::shared_ptr<Material> m_ModelMaterial1;
	std::shared_ptr<Material> m_ModelMaterial2;
	std::shared_ptr<Material> m_ModelMaterial3;



	std::shared_ptr<Mesh> m_Mesh;
	//Mesh* m_Mesh = nullptr;
	Mesh* m_Mesh2 = nullptr;

	std::shared_ptr<Mesh> m_Triangle;

	std::shared_ptr<Mesh> m_Quad;
	std::shared_ptr<Shader> m_UnlitShader;
	std::shared_ptr<Material> m_QuadMaterial;



	std::shared_ptr<Material> m_Material;
	std::shared_ptr<Material> m_Material2;

	std::shared_ptr<Texture> m_RenderTargetSample;


	std::shared_ptr<Texture> m_CubeMap;
	std::shared_ptr<Shader> m_SkyShader;
	std::shared_ptr<Material> m_SkyMaterial;

	std::shared_ptr<Shader> m_Shader;
	std::shared_ptr<Shader> m_Shader2;
};

