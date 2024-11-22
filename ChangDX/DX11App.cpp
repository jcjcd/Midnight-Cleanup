
#include "framework.h"
#include "C3DApp.h"

#include <vector>
#include <filesystem>

#include <directxtk/SimpleMath.h>
#include <string>

using Vector2 = DirectX::SimpleMath::Vector2;
using Vector3 = DirectX::SimpleMath::Vector3;
using Vector4 = DirectX::SimpleMath::Vector4;
using Matrix = DirectX::SimpleMath::Matrix;
using Quaternion = DirectX::SimpleMath::Quaternion;

#include "windows.h"

#include "../Animavision/Mesh.h"
#include "../Animavision/ModelLoader.h"
#include "../Animavision/Shader.h"
#include "../Animavision/Material.h"
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;

C3DApp::C3DApp() :
	m_hWnd(nullptr),
	m_ScreenWidth(0),
	m_ScreenHeight(0)
{
	LPTSTR str = new TCHAR[256];
	GetPrivateProfileString(L"Graphics", L"ScreenWidth", nullptr, str, 256, L"./config.ini");
	lstrlen(str) <= 0 ? (m_ScreenWidth = 1920) : (m_ScreenWidth = _wtoi(str));
	GetPrivateProfileString(L"Graphics", L"ScreenHeight", nullptr, str, 256, L"./config.ini");
	lstrlen(str) <= 0 ? (m_ScreenHeight = 1080) : (m_ScreenHeight = _wtoi(str));
	GetPrivateProfileString(L"Graphics", L"GraphicAPI", nullptr, str, 256, L"./config.ini");
	if (lstrlen(str) <= 0) {
		m_API = Renderer::API::DirectX12;
	}
	else {
		if (wcscmp(str, L"DirectX11") == 0) {
			m_API = Renderer::API::DirectX11;
		}
		else if (wcscmp(str, L"DirectX12") == 0) {
			m_API = Renderer::API::DirectX12;
		}
		else {
			m_API = Renderer::API::DirectX12;
		}
	}
	delete[] str;

}

C3DApp::~C3DApp()
{
	WritePrivateProfileString(L"Graphics", L"ScreenWidth", std::to_wstring(m_ScreenWidth).c_str(), L"./config.ini");
	WritePrivateProfileString(L"Graphics", L"ScreenHeight", std::to_wstring(m_ScreenHeight).c_str(), L"./config.ini");
	std::wstring str;
	if (m_API == Renderer::API::DirectX11) {
		str = L"DirectX11";
	}
	else if (m_API == Renderer::API::DirectX12) {
		str = L"DirectX12";
	}
	WritePrivateProfileString(L"Graphics", L"GraphicAPI", str.c_str(), L"./config.ini");
}

void C3DApp::CalculateFrameStats()
{
	static int frameCnt = 0;
	static double elapsedTime = 0.0f;
	double totalTime = m_Timer.GetTotalSeconds();
	frameCnt++;

	// Compute averages over one second period.
	if ((totalTime - elapsedTime) >= 1.0f)
	{
		float diff = static_cast<float>(totalTime - elapsedTime);
		float fps = static_cast<float>(frameCnt) / diff; // Normalize to an exact second.

		frameCnt = 0;
		elapsedTime = totalTime;

		float MRaysPerSecond = (m_ScreenWidth * m_ScreenHeight * fps) / static_cast<float>(1e6);

		wstringstream windowText;

		windowText << setprecision(2) << fixed
			<< L"    fps: " << fps << L"     ~Million Primary Rays/s: " << MRaysPerSecond
			<< L"    GPU[";
		SetCustomWindowText(windowText.str().c_str());
	}
}

void C3DApp::SetCustomWindowText(LPCWSTR text)
{
	std::wstring windowText = m_Title + L": " + text;
	SetWindowText(m_hWnd, windowText.c_str());
}

HRESULT C3DApp::Initialize()
{
	WNDCLASSEXW wcex{};


	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = HINST_THISCOMPONENT;
	wcex.hIcon = nullptr;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = nullptr;
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = L"Chang3DEngine";
	wcex.hIconSm = nullptr;

	RegisterClassExW(&wcex);


	m_hWnd = CreateWindowW(
		L"Chang3DEngine",
		L"2023 Chang3D Engine Demo",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		0,
		m_ScreenWidth,
		m_ScreenHeight,
		nullptr,
		nullptr,
		HINST_THISCOMPONENT,
		this);

	RECT rc = { 0, 0, m_ScreenWidth , m_ScreenHeight };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, false);
	SetWindowPos(m_hWnd, NULL,
		0, 0,
		(rc.right - rc.left), (rc.bottom - rc.top), // 변경할 크기(가로, 세로) 
		SWP_NOZORDER | SWP_NOMOVE	// 깊이변경 금지 | 이동 금지
	);

	ShowWindow(m_hWnd, SW_SHOWNORMAL);
	UpdateWindow(m_hWnd);

	m_Renderer = Renderer::Create(m_hWnd, m_ScreenWidth, m_ScreenHeight, m_API);

	// 여러 개의  texture2d를 texture2d array로
	std::vector<std::shared_ptr<Texture>> textures;

	textures.push_back(m_Renderer->CreateLightmapTexture("Lightmap-0_comp_dir.png"));
	textures.push_back(m_Renderer->CreateLightmapTexture("Lightmap-1_comp_dir.png"));
	textures.push_back(m_Renderer->CreateLightmapTexture("Lightmap-2_comp_dir.png"));
	textures.push_back(m_Renderer->CreateLightmapTexture("Lightmap-3_comp_dir.png"));
	textures.push_back(m_Renderer->CreateLightmapTexture("Lightmap-4_comp_dir.png"));
	textures.push_back(m_Renderer->CreateLightmapTexture("Lightmap-5_comp_dir.png"));
	textures.push_back(m_Renderer->CreateLightmapTexture("Lightmap-6_comp_dir.png"));
	textures.push_back(m_Renderer->CreateLightmapTexture("Lightmap-7_comp_dir.png"));
	textures.push_back(m_Renderer->CreateLightmapTexture("Lightmap-8_comp_dir.png"));
	textures.push_back(m_Renderer->CreateLightmapTexture("Lightmap-9_comp_dir.png"));

	m_Renderer->SaveLightmapTextureArray(textures, "/Resources/lightmap09");

	ModelParserFlags flags = ModelParserFlags::NONE;
	flags |= ModelParserFlags::TRIANGULATE;
	flags |= ModelParserFlags::MAKE_LEFT_HANDED;
	flags |= ModelParserFlags::GEN_SMOOTH_NORMALS;
	flags |= ModelParserFlags::GEN_UV_COORDS;
	flags |= ModelParserFlags::CALC_TANGENT_SPACE;
	//flags |= ModelParserFlags::GEN_BOUNDING_BOXES;
	flags |= ModelParserFlags::FLIP_WINDING_ORDER;
	flags |= ModelParserFlags::FLIP_UVS;

	// for animation + triangulate
	flags |= ModelParserFlags::LIMIT_BONE_WEIGHTS;
	flags |= ModelParserFlags::JOIN_IDENTICAL_VERTICES;

	/*m_Shader = m_Renderer->LoadShader("./Shaders/UI.hlsl");
	m_Material = Material::Create(m_Shader);

	auto quadMesh = MeshGenerator::CreateQuad({ -1, -1 }, { 1,1 }, 0);
	m_Renderer->AddMesh(quadMesh);
	m_Quad = m_Renderer->GetMesh("Quad");

	std::shared_ptr<Texture> texture = m_Renderer->GetUITexture("icon_0000_axe.png");
	m_Material->SetTexture("gDiffuseMap", texture);*/

#pragma region failed example
	/// 실패 예제
	/*std::shared_ptr<Shader> fireParticleStreamOutShader = m_Renderer->LoadShader("fireParticleStreamOut.hlsl");
	std::shared_ptr<Shader> fireParticleDrawShader = m_Renderer->LoadShader("fireParticleDraw.hlsl");
	std::shared_ptr<Texture> fireTextureArray = m_Renderer->CreateTexture2DArray("./Resources/Textures/flare0.dds");

	m_ParticleSystem = std::make_shared<LunaParticleSystem>(*m_Renderer.get(), fireParticleStreamOutShader.get(), fireParticleDrawShader.get(), fireTextureArray.get(), 100);*/
#pragma endregion

	/*m_ParticleManager = std::make_shared<ParticleManager>(m_Renderer.get());

	ParticleInfo particleInfo = {};
	particleInfo.MainData.StartSize.x = 10.f;
	particleInfo.MainData.StartSpeed.x = 50.f;
	particleInfo.MainData.StartColor0 = { 1.0f, 0.3f, 0.3f, 1.0f };
	particleInfo.ShapeData.ShapeType = ParticleInfo::Shape::EShape::Sphere;
	particleInfo.ShapeData.Scale = { 5.0f, 5.0f, 5.0f };
	particleInfo.ShapeData.ArcInDegree = 242.0f;
	particleInfo.ShapeData.Rotation = { 10.0f, 10.0f, 10.0f };
	particleInfo.ShapeData.Scale = { 2.0f, 4.0f, 3.0f };
	particleInfo.ShapeData.Radius = 10.f;

	particleInfo.LimitVelocityOverLifeTimeData.IsUsed = true;
	particleInfo.LimitVelocityOverLifeTimeData.Speed = 5.f;
	particleInfo.LimitVelocityOverLifeTimeData.Dampen = 0.5f;
	auto particleObj = m_ParticleManager->CreateParticleObject(m_Renderer.get(), particleInfo);
	particleObj->SetTransform(Matrix::CreateScale(1.0f, 2.0f, 3.0f) * Matrix::CreateFromYawPitchRoll(1.24f, 2.46f, 3.68f) * Matrix::CreateTranslation({ 0.0f, 0.0f,0.0f }));
	m_ParticleObjects.push_back(particleObj);

	particleInfo.EmissionData.ParticlePerSecond = 200.f;
	particleObj = m_ParticleManager->CreateParticleObject(m_Renderer.get(), particleInfo);
	particleObj->SetTransform(Matrix::CreateTranslation(0.0f, 10.0f, 0.0f));
	m_ParticleObjects.push_back(particleObj);*/
	return S_OK;
}

void C3DApp::RunMessageLoop()
{
	static Vector3 eyePos(0, 0, +10);
	static Quaternion quat = Quaternion::Identity;
	static Vector3 boxPos = Vector3(0, 0, 0);
	static Vector4 color = Vector4(1, 1, 1, 1);
	static bool isOn = true;

	MSG msg;
	float aspect = m_ScreenWidth / (float)m_ScreenHeight;

	while (true) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) break;

			DispatchMessage(&msg);
		}
		else
		{
			m_Timer.Tick();
			CalculateFrameStats();

			auto view = Matrix::CreateFromQuaternion(quat) * Matrix::CreateTranslation(eyePos);
			Matrix proj = DirectX::XMMatrixPerspectiveFovLH(45.0f * (DirectX::XM_PI / 180.0f), m_ScreenWidth / (float)m_ScreenHeight, 1.0f, 10000.0f);
			proj = DirectX::XMMatrixOrthographicLH(aspect * 25, 25.0f, 0, 1);
			auto world = Matrix::CreateTranslation(boxPos);

			if (GetAsyncKeyState(VK_UP) & 0x8000)
			{
				boxPos.y += 0.1f * 10;
			}
			if (GetAsyncKeyState(VK_DOWN) & 0x8000)
			{
				boxPos.y -= 0.1f * 10;
			}
			if (GetAsyncKeyState(VK_LEFT) & 0x8000)
			{
				boxPos.x -= 0.1f * 10;
			}
			if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
			{
				boxPos.x += 0.1f * 10;
			}
			if (GetAsyncKeyState('Q') & 0x8000)
			{
				boxPos.z -= 0.1f;
			}
			if (GetAsyncKeyState('E') & 0x8000)
			{
				boxPos.z += 0.1f;
			}
			if (GetAsyncKeyState('X') & 0x8000)
			{
				isOn = !isOn;
			}

			m_Renderer->BeginRender();

			/*m_Renderer->SetViewport(m_ScreenWidth, m_ScreenHeight);
			m_Renderer->SetRenderTargets(1, nullptr);


			m_Renderer->ApplyRenderState(BlendState::ALPHA_BLEND, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_DISABLED);

			m_Shader->MapConstantBuffer(m_Renderer->GetContext());
			m_Shader->SetMatrix("gWorld", world);
			m_Shader->SetMatrix("gViewProj", Matrix::Identity * proj);
			m_Shader->SetFloat4("gColor", color);
			m_Shader->SetInt("isOn", isOn);
			m_Shader->SetFloat("layerDepth", 0.0f);
			m_Shader->UnmapConstantBuffer(m_Renderer->GetContext());

			m_Renderer->Submit(*m_Quad, *m_Material);*/

#pragma region failed example
			/// 실패 예제
			/*m_ParticleSystem->SetEyePos(camera.GetPosition());
			static float totalTime = 0.0f;
			m_ParticleSystem->SetEmitPos({ 10.0f, 0.0f, 0.0f });
			m_ParticleSystem->SetEmitDir({ 0.0f, 1.0f, 0.0f });
			m_ParticleSystem->Update(0.016f, totalTime);
			totalTime += 0.016f;
			m_ParticleSystem->Render(*m_Renderer.get(), camera.GetViewProj());*/
#pragma endregion

			/// particle system test
			// Render 함수에 카메라 뷰 프로젝션 행렬을 넣어야 함
			// 일단 내가 만든 임의의 카메라 추가해서 진행

			/*static float s_particlePerSecond = 10.0f;
			s_particlePerSecond += 0.001f;

			for (auto& obj : m_ParticleObjects)
			{
				ParticleInfo particleInfo = {};
				particleInfo.MainData.StartSpeedOption = ParticleInfo::EOption::RandomBetweenTwoConstants;
				particleInfo.MainData.StartSpeed = { 1.0f, s_particlePerSecond };

				particleInfo.MainData.StartSizeOption = ParticleInfo::EOption::RandomBetweenTwoConstants;
				particleInfo.MainData.StartSize = { 1.0f, s_particlePerSecond };

				particleInfo.EmissionData.ParticlePerSecond = s_particlePerSecond;

				particleInfo.ShapeData.ArcInDegree = s_particlePerSecond * 10.0f;
				particleInfo.ShapeData.Radius = 1.0f + s_particlePerSecond;

				if (GetAsyncKeyState('P') & 0x8000)
				{
					particleInfo.RenderData.TexturePath = "./Resources/Textures/Particle00.png";
				}
				else
				{
					particleInfo.RenderData.TexturePath = "./Resources/Textures/Particle06.png";
				}

				obj->SetFrameTime(0.016f);
				obj->SetIsRenderDebug(true);

				if (GetAsyncKeyState('O') & 0x8000)
				{
					obj->SetIsEmit(true);
				}
				else
				{
					obj->SetIsEmit(false);
				}

				if (GetAsyncKeyState('I') & 0x8000)
				{
					obj->SetIsReset(true);
				}
			}*/

			// camera data missing
			//m_ParticleManager->Execute(m_Renderer.get(), data);


			m_Renderer->EndRender();
		}
	}

}

void C3DApp::Finalize()
{

}


LRESULT CALLBACK C3DApp::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch (message)
	{
		// 무효화 영역 발생
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: 여기에 hdc를 사용하는 그리기 코드를 추가합니다...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_SIZE:
	{
		int a = 0;
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
