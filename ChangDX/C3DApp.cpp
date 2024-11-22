
#include "framework.h"
#include "C3DApp.h"

#include <vector>

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
#include "../Animavision/ShaderResource.h"

#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;

struct PerLight
{
	Vector4 Position[4];
	Vector4 Color[4];
};

C3DApp::C3DApp() :
	m_hWnd(nullptr),
	m_ScreenWidth(0),
	m_ScreenHeight(0),
	m_Title(L"2024 Chang3D Engine Demo")
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

class Transform;


struct SceneConstantBuffer
{
	Matrix projectionToWorld;
	Vector4 cameraPosition;
	Vector4 lightPosition;
	Vector4 lightAmbientColor;
	Vector4 lightDiffuseColor;
};

HRESULT C3DApp::Initialize()
{
#pragma region Window
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
	wcex.lpszClassName = m_Title.c_str();
	wcex.hIconSm = nullptr;

	RegisterClassExW(&wcex);


	m_hWnd = CreateWindowW(
		m_Title.c_str(),
		L"2024 Chang3D Engine Demo",
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
#pragma endregion

	m_Renderer = Renderer::Create(m_hWnd, m_ScreenWidth, m_ScreenHeight, m_API, true);

	m_Quad = MeshGenerator::CreateQuad({ -1, -1 }, { 1, 1 }, 0);
	m_Quad->CreateBuffers(m_Renderer.get());

	m_Triangle = MeshGenerator::CreateTriangle();
	m_Triangle->CreateBuffers(m_Renderer.get());

	m_Renderer->LoadMeshesFromDrive("./Resources/Models/Test");

	ModelParserFlags flags = ModelParserFlags::NONE;
	flags |= ModelParserFlags::TRIANGULATE;
	//flags |= ModelParserFlags::GEN_NORMALS;
	flags |= ModelParserFlags::GEN_SMOOTH_NORMALS;
	flags |= ModelParserFlags::GEN_UV_COORDS;
	flags |= ModelParserFlags::CALC_TANGENT_SPACE;
	flags |= ModelParserFlags::GEN_BOUNDING_BOXES;
	// 왼손 좌표계 바꾸려면 밑에꺼 3개 다해야됨
	flags |= ModelParserFlags::MAKE_LEFT_HANDED;
	flags |= ModelParserFlags::FLIP_UVS;
	flags |= ModelParserFlags::FLIP_WINDING_ORDER;
	flags |= ModelParserFlags::LIMIT_BONE_WEIGHTS;

	auto k = ModelLoader::CreateForEntity("./Resources/Models/Test/warrok.fbx", flags);

	auto k2 = ModelLoader::LoadAnimation("./Resources/Models/Test/warrok.fbx", flags);

	m_Mesh = m_Renderer->GetMesh("Sphere");

 	//m_Renderer->AddBottomLevelAS(*m_Renderer->GetMesh("Warrok"), true);
// 
// 	//m_Renderer->AddBottomLevelAS(*m_Renderer->GetMesh("head"), true);
// 
// 	m_Renderer->AddBottomLevelAS(*m_Renderer->GetMesh("Sphere"), true);
// 
// 	m_Renderer->AddBottomLevelAS(*m_Renderer->GetMesh("Cube"), true);


// 	auto mat11 = m_MaterialLibrary.LoadFromFile("./Resources/Materials/rtpbr.material");
// 	auto mat22 = m_MaterialLibrary.LoadFromFile("./Resources/Materials/fish.material");
// 	auto mat33 = m_MaterialLibrary.LoadFromFile("./Resources/Materials/rtpbr.material");

	auto mat11 = m_MaterialLibrary.LoadFromFile("./Resources/Materials/cartbot.material");
	auto mat22 = m_MaterialLibrary.LoadFromFile("./Resources/Materials/carttop.material");
	auto mat33 = m_MaterialLibrary.LoadFromFile("./Resources/Materials/rtpbr.material");

	m_Materials1.push_back(mat11);

	m_Materials2.push_back(mat22);
	m_Materials2.push_back(mat22);

	m_Materials3.push_back(mat33);

	m_Renderer->LoadMaterialsFromDrive("./Resources/Materials/");

// 	Matrix world1 =
// 		Matrix::CreateScale(100.f, 100.f, 100.f) *
// 		Matrix::Identity *
// 		Matrix::CreateTranslation(Vector3(200, 0, 50));
// 	m_Renderer->AddMeshToScene(m_Renderer->GetMesh("Cube"), m_Materials1, &world1.m[0][0], true);

// 
// 	Matrix world1 = Matrix::Identity;
// 	m_Renderer->AddMeshToScene(m_Renderer->GetMesh("add.main_low.002"), m_Materials1, &world1.m[0][0], true);
// 
// 	Matrix world2 = Matrix::Identity;
// 	m_Renderer->AddMeshToScene(m_Renderer->GetMesh("add.main_low.001"), m_Materials2, &world2.m[0][0], true);

	//Matrix world2 = Matrix::CreateTranslation(Vector3(-500, 0, 0));
// 	Matrix world2 =
// 		Matrix::CreateScale(100.f, 100.f, 100.f) *
// 		Matrix::Identity *
// 		Matrix::CreateTranslation(Vector3(-200, 0, 50));
// 	m_Renderer->AddMeshToScene(m_Renderer->GetMesh("G_Fish67"), m_Materials2, &world2.m[0][0], true);


// 	Matrix world3 =
// 		Matrix::CreateScale(100.f, 100.f, 100.f) *
// 		Matrix::Identity *
// 		Matrix::CreateTranslation(Vector3(500, 0, -50));
// 	m_Renderer->AddMeshToScene(m_Renderer->GetMesh("Sphere"), m_Materials3, &world3.m[0][0], true);

// 	Matrix world4 = Matrix::CreateTranslation(Vector3(-100, -100, 300));
// 	m_Renderer->AddMeshToScene(m_Renderer->GetMesh("Cube"), m_Materials3, &world4.m[0][0], true);


	m_Shader = m_Renderer->LoadShader("./Shaders/DefaultPBR.hlsl");

	m_RenderTargetSample = m_Renderer->CreateEmptyTexture("sample", Texture::Type::Texture2D, 1920, 1080, 1, Texture::Format::R8G8B8A8_UNORM, Texture::Usage::RTV | Texture::Usage::UAV);


	m_SkyShader = m_Renderer->LoadShader("./Shaders/Sky.hlsl");
	m_SkyMaterial = Material::Create(m_SkyShader);
	m_CubeMap = Texture::Create(m_Renderer.get(), "./Resources/Textures/desertcube1024.dds", Texture::Type::TextureCube);

	m_UnlitShader = m_Renderer->LoadShader("./Shaders/unlit.hlsl");
	m_QuadMaterial = Material::Create(m_UnlitShader);

	//auto texture = m_Renderer->CreateTexture("Resources/Textures/defaultmagentapng.png");
	//m_QuadMaterial->SetTexture("gDiffuseMap", texture);

	m_QuadMaterial->SetTexture("gDiffuseMap", m_Renderer->GetColorRt());


	return S_OK;
}

void C3DApp::RunMessageLoop()
{
	static Vector3 eyePos(0, 0, +10);
	static Quaternion quat = Quaternion::Identity;
	static Vector3 boxPos = Vector3(0, 0, 0);
	static PerLight perLight = { {Vector4(0, 0, -200, 1), Vector4(0, 0, 20, 1), Vector4(0, 200, 0, 1), Vector4(0, -200, 0, 1)},
		{ Vector4(1, 1, 1, 1), Vector4(1, 1, 1, 1), Vector4(100, 100, 100, 100), Vector4(1, 1, 1, 1) } };

	MSG msg;

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
#pragma region Input
			auto view = Matrix::CreateFromQuaternion(quat) * Matrix::CreateTranslation(eyePos);
			auto viewinv = view.Invert();
			Matrix proj = DirectX::XMMatrixPerspectiveFovLH(45.0f * (DirectX::XM_PI / 180.0f), m_ScreenWidth / (float)m_ScreenHeight, 1.0f, 10000.0f);
			auto viewProj = viewinv * proj;

			if (GetAsyncKeyState(VK_UP) & 0x8000)
			{
				boxPos.y += 0.1f;
			}
			if (GetAsyncKeyState(VK_DOWN) & 0x8000)
			{
				boxPos.y -= 0.1f;
			}
			if (GetAsyncKeyState(VK_LEFT) & 0x8000)
			{
				boxPos.x -= 0.1f;
			}
			if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
			{
				boxPos.x += 0.1f;
			}
			if (GetAsyncKeyState('W') & 0x8000)
			{
				eyePos += -view.Forward();
			}
			if (GetAsyncKeyState('S') & 0x8000)
			{
				eyePos -= -view.Forward();
			}
			if (GetAsyncKeyState('Q') & 0x8000)
			{
				boxPos.z -= 0.1f;
			}
			if (GetAsyncKeyState('E') & 0x8000)
			{
				boxPos.z += 0.1f;
			}
			if (GetAsyncKeyState('A') & 0x8000)
			{
				eyePos -= view.Right();
			}
			if (GetAsyncKeyState('D') & 0x8000)
			{
				eyePos += view.Right();
			}
			// 1, 3 으로 카메라를 업다운 시킬거다
			if (GetAsyncKeyState('1') & 0x8000)
			{
				eyePos += view.Up();
			}
			if (GetAsyncKeyState('3') & 0x8000)
			{
				eyePos -= view.Up();
			}


			// z, c 로 고개를 돌리고싶다.
			if (GetAsyncKeyState('Z') & 0x8000)
			{
				quat = Quaternion::CreateFromYawPitchRoll(0.001f, 0, 0) * quat;
			}
			if (GetAsyncKeyState('C') & 0x8000)
			{
				quat = Quaternion::CreateFromYawPitchRoll(-0.001f, 0, 0) * quat;
			}
#pragma endregion



			m_Renderer->BeginRender();

			m_Renderer->FlushRaytracingData();

// 			Matrix world1 = Matrix::Identity;
// 			m_Renderer->AddMeshToScene(m_Renderer->GetMesh("add.main_low.002"), m_Materials1, &world1.m[0][0], true);
// 
// 			Matrix world2 = Matrix::Identity;
// 			m_Renderer->AddMeshToScene(m_Renderer->GetMesh("add.main_low.001"), m_Materials2, &world2.m[0][0], true);

			m_Renderer->SetCamera(viewinv, proj, eyePos, -view.Forward(), view.Up(), 1.0f, 10000.f);

			m_Renderer->DoRayTracing();

			m_Renderer->SetViewport(m_ScreenWidth, m_ScreenHeight);
			m_Renderer->SetRenderTargets(1, nullptr);

			//m_Renderer->Clear(clr);


			m_UnlitShader->SetMatrix("gWorld", Matrix::Identity);
			m_UnlitShader->SetMatrix("gView", Matrix::Identity);
			m_UnlitShader->SetMatrix("gViewProj", Matrix::Identity);

			m_Renderer->Submit(*m_Quad, *m_QuadMaterial, 0, PrimitiveTopology::TRIANGLELIST, 1);


			m_Renderer->EndRender();
		}
	}

}

void C3DApp::Finalize()
{
	//delete m_Model;
}

#pragma region WindowProc
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
#pragma endregion