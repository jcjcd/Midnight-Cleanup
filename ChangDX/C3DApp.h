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

// Test TODO ����� 
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

	/// �뵵: â Ŭ������ ��� �� �� �ν��Ͻ� �ڵ��� �����ϰ� �� â�� ����ϴ�.
	/// �ּ�:
	///      �� �Լ��� ���� �ڵ��������� ��������� �����ϰ� �� ���α׷� â�� ���� ���� ǥ���մϴ�.
	HRESULT Initialize();

	/// PeekMessage�� ���� �������� �޽��� ������ ó���Ѵ�.
	void RunMessageLoop();

	/// ���� �� ����� �ؾ� �� �͵��� ��Ƴ��´�.
	void Finalize();

	/// �Լ�: WndProc(HWND, UINT, WPARAM, LPARAM)
	/// �뵵: �� â�� �޽����� ó���մϴ�.
	static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

private:
	HWND m_hWnd;
	// ȭ�� ũ��
	int m_ScreenWidth;
	int m_ScreenHeight;

	StepTimer m_Timer;
	std::wstring m_Title;

	inline static bool m_bSizechanged = false;
	// ������
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

