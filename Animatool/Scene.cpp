#include "pch.h"
#include "Scene.h"

#include "ToolProcess.h"
#include "SceneViewNavBar.h"


#include <Animacore/Scene.h>
#include <Animacore/CoreComponents.h>
#include <Animacore/RenderComponents.h>

#include <Animavision/Mesh.h>
#include <Animavision/Shader.h>
#include <Animavision/Material.h>
#include <Animavision/Renderer.h>

#include <Animacore/RenderSystems.h>

#include <ImGuizmo.h>
#include <imgui_internal.h>

#include <Animacore/Inputsystem.h>

#include <Animacore/CorePhysicsComponents.h>

#include "../midnight_cleanup/McComponents.h"
#include "Animacore/CoreTagsAndLayers.h"
#include "midnight_cleanup/McTagsAndLayers.h"

namespace DeferredSlot
{
	enum
	{
		Albedo,
		ORM,
		Normal,
		PositionW,
		Emissive,
		PreCalculatedLight,
		Mask,

		Count,
	};
}

bool IsCursorVisible()
{
	CURSORINFO ci = { sizeof(CURSORINFO) };
	if (GetCursorInfo(&ci))
		return ci.flags & CURSOR_SHOWING;
	return false;
}

tool::Scene::Scene(entt::dispatcher& dispatcher, Renderer* renderer)
	: Panel(dispatcher)
	, _renderer(renderer)
{
	_camera.Orthographic = false;
	_camera.FarClipPlane = 500.0f;
	_camera.NearClipPlane = 0.1f;
	_camera.FieldOfView = 45.0f * (DirectX::XM_PI / 180.0f);
	_camera.Position = Vector3(0, 0, 0);
	_camera.Rotation = Quaternion::Identity;
	_camera.size = 2;
	_camera.Aspect = _width / (float)_height;
	_camera.frustum.CreateFromMatrix(_camera.frustum, _camera.ProjectionMatrix);
	_camera.frustum.Transform(_camera.frustum, _camera.ViewMatrix.Invert());

	_navBar = new SceneViewNavBar{ dispatcher };

	// event
	_dispatcher->sink<OnToolSelectEntity>().connect<&Scene::selectEntity>(this);
	_dispatcher->sink<OnToolPlayScene>().connect<&Scene::playScene>(this);
	_dispatcher->sink<OnToolStopScene>().connect<&Scene::stopScene>(this);
	_dispatcher->sink<OnToolDoubleClickEntity>().connect<&Scene::doubleClickEntity>(this);

	_dispatcher->sink<OnToolModifyRenderer>().connect<&Scene::modifyRenderer>(this); // 생성, 수정

	_dispatcher->sink<OnToolApplyCamera>().connect<&Scene::applyCamera>(this);


	if (_renderer->IsRayTracing())
	{
		// 레이트레이싱에서 이부분이 필요하다.
		_dispatcher->sink<OnToolRemoveComponent>().connect<&Scene::modifyRenderer>(this); // 삭제
		_dispatcher->sink<OnToolDestroyEntity>().connect<&Scene::modifyRenderer>(this); // 삭제
		_dispatcher->sink<OnToolLoadScene>().connect<&Scene::modifyRenderer>(this); // 로드
		_dispatcher->sink<OnToolLoadPrefab>().connect<&Scene::modifyRenderer>(this); // 저장

		InitializeRaytracingResources();
	}
	else
	{
		InitializeShaderResources();
	}

	for (auto&& [key, val] : *_renderer->GetMaterials())
	{
		if (key.find("default.material") != std::string::npos)
		{
			_defaultMaterialName = key;
			break;
		}
	}
}

tool::Scene::~Scene()
{
	delete _navBar;

	_deferredTextures.clear();
	_deferredMaterial.reset();
	_dLightDepthTexture.reset();
	_dDepthMaterial.reset();
	_quadMesh.reset();
	_quadMaterial.reset();
	_outlineComputeMaterial.reset();
	_pickingMaterial.reset();
	_pickingTexture.reset();
	_renderTarget.reset();
}

void tool::Scene::InitializeRaytracingResources()
{
	_quadMaterial = Material::Create(_renderer->LoadShader("./Shaders/unlit.hlsl"));
	_quadMaterial->SetTexture("gDiffuseMap", _renderer->GetColorRt());

	auto quadMesh = MeshGenerator::CreateQuad({ -1, -1 }, { 1, 1 }, 0);

	_renderer->AddMesh(quadMesh);
	_quadMesh = _renderer->GetMesh("Quad");

	// 빈 셰이더 바인딩 테이블을 빌드한다.
	_renderer->AddMeshToScene(nullptr, {}, nullptr, true);

}

void tool::Scene::InitializeShaderResources()
{
	initLightResources();
	initDeferredResources();
	initPostProcessResources();
	initOITResources();

	_deferredMaterial->SetTexture("gDecalOutputAlbedo", _decalOutputAlbedo);
	_deferredMaterial->SetTexture("gDecalOutputORM", _decalOutputORM);


	float pickingClearColor[] = { -1, -1, -1, -1 };

	_pickingTexture = _renderer->CreateEmptyTexture("PickingRT", Texture::Type::Texture2D, _width, _height, 1, Texture::Format::R32_SINT, Texture::Usage::RTV, pickingClearColor);

	_pickingMaterial = Material::Create(_renderer->LoadShader("./Shaders/pickingVSPS.hlsl"));
	_outlineComputeMaterial = Material::Create(_renderer->LoadShader("./Shaders/CS_outline.hlsl"));

	_outlineComputeMaterial->SetTexture("pickingRTV", _pickingTexture);
	_outlineComputeMaterial->SetUAVTexture("finalRenderTarget", _renderTarget);

	_uiMaterial = Material::Create(_renderer->LoadShader("./Shaders/UI.hlsl"));



}

void tool::Scene::RenderPanel(float deltaTime)
{
	ImGuiWindowFlags wflags = 0;
	wflags |= ImGuiWindowFlags_MenuBar
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoBackground
		| ImGuiWindowFlags_NoScrollWithMouse;

	if (!_isPlaying)
	{

		Vector2 mousePos = { ImGui::GetMousePos().x , ImGui::GetMousePos().y };

		DirectX::SimpleMath::Rectangle rect = {
			(long)viewportPos.x,
			(long)viewportPos.y + 44,
			(long)viewportSize.x,
			(long)viewportSize.y - 44
		};

		if (_isFocused
			&& rect.Contains((long)mousePos.x, (long)mousePos.y)
			&& ImGui::IsMouseClicked(ImGuiMouseButton_Left)
			&& !ImGuizmo::IsOver())
		{
			pickItemByCPU((int)mousePos.x, (int)mousePos.y - 22);
		}

		if (_isFocused
			&& rect.Contains((long)mousePos.x, (long)mousePos.y)
			&& ImGui::IsMouseDown(ImGuiMouseButton_Right)
			&& !_isRightClicked)
		{
			_isSceneRightClicked = true;
		}

		if (ImGui::IsKeyReleased(ImGuiKey_MouseRight))
		{
			_isSceneRightClicked = false;
		}
		_isRightClicked = ImGui::IsMouseDown(ImGuiMouseButton_Right);

		// 카메라 이동
		if (_currentGizmoOperation == Hand && ImGui::IsMouseDown(ImGuiMouseButton_Left))
			_camera.CameraDrag(deltaTime);
		else if (_isSceneRightClicked)
			_camera.CameraMoveRotate(deltaTime);

		_camera.UpdateMousePos(mousePos);


		// ImGui의 입력 처리
		// 우클릭 눌릴때만
		if (ImGui::IsKeyPressed(ImGuiKey_MouseRight))
		{
			float wheel = ImGui::GetIO().MouseWheel;

			if (wheel > 0.0f)  // 휠을 위로 스크롤
			{
				_camera.Speed += _camera.SpeedDesplacement;
			}
			else if (wheel < 0.0f)  // 휠을 아래로 스크롤
			{
				_camera.Speed -= _camera.SpeedDesplacement;
			}
			_camera.Speed = std::clamp(_camera.Speed, _camera.MinSpeed, _camera.MaxSpeed);
		}




		// 카메라 슝
		if (_isDoubleClicked)
		{
			Vector3 updatedPos = Vector3::Lerp(_camera.Position, _targetWorldPosition + _camera.GetWorldMatrix().Forward() * 10.f * _scaleAvg, _doubleClickTime);
			_camera.Position = updatedPos;
			_doubleClickTime += deltaTime * 2.0f;

			if (_doubleClickTime >= 1.0f)
			{
				_isDoubleClicked = false;
				_doubleClickTime = 0.0f;
			}
		}

		if (_renderer->IsRayTracing())
		{
			RenderRaytracingScene(*ToolProcess::scene);
		}
		else
		{
			RenderScene(deltaTime, *ToolProcess::scene);
		}
	}


	if (ImGui::Begin("Scene", 0, wflags))
	{
		_isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
		_navBar->RenderPlayPauseButton();
		_navBar->RenderToolTypePanel(_currentGizmoOperation, _currentMode, _useSnap, _snap, _isRightClicked, this);

		auto current = ImGui::GetCurrentContext();
		auto pos = current->CurrentWindow->Pos;
		auto size = current->CurrentWindow->Size;
		auto idealSize = ImVec2(size.x, size.y - 3);
		auto borderSize = current->CurrentWindow->WindowBorderSize;
		auto maxpos = ImVec2(pos.x + idealSize.x, pos.y + idealSize.y);

		if (viewportSize.x != idealSize.x || viewportSize.y != idealSize.y)
		{
			_camera.Aspect = idealSize.x / idealSize.y;
		}

		void* myTex = nullptr;
		if (!_isPlaying)
		{
			myTex = _renderer->GetShaderResourceView();
		}
		else
		{
			auto&& rtComponent = ToolProcess::scene->GetRegistry()->ctx().get<core::RenderResources>();
			if (rtComponent.renderTarget)
			{
				myTex = rtComponent.renderTarget->GetShaderResourceView();
				LONG width = rtComponent.renderTarget->GetWidth();
				LONG height = rtComponent.renderTarget->GetHeight();
				auto viewPortRatio = (float)width / (float)height;
				auto inGameViewPortRatio = idealSize.x / idealSize.y;
				ImVec2 realSize = {};
				if (viewPortRatio > inGameViewPortRatio)
				{
					// 뷰포트의 위아래가 남는다.
					realSize.x = idealSize.x;
					realSize.y = idealSize.x / viewPortRatio;
					pos.y += (idealSize.y - realSize.y) * 0.5f;
				}
				else
				{
					// 뷰포트의 좌우가 남는다
					realSize.y = idealSize.y;
					realSize.x = idealSize.y * viewPortRatio;
					pos.x += (idealSize.x - realSize.x) * 0.5f;
				}

				maxpos = ImVec2(pos.x + realSize.x, pos.y + realSize.y);

			}

			// 창 : 이거 최적화 할것. if를 좀 뒤집자
			// 플레이중일때 다른 창에 포커싱이 있다가 씬을 클릭하면 포커싱을 다시 가져오면서 마우스 커서를 숨김
			if (ImGui::GetMousePos().x > pos.x && ImGui::GetMousePos().x < maxpos.x
				&& ImGui::GetMousePos().y > pos.y + 22 && ImGui::GetMousePos().y < maxpos.y)
			{
				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					if (!_isMouseCursorPrevVisible)
					{
						while (ShowCursor(false) >= 0);
						global::mouseMode = global::MouseMode::Lock;
					}
				}
			}
			//ShowCursor(false);

			// 플레이중일때 esc를 누르면 마우스 커서를 보이게한다.
			if (ImGui::IsKeyPressed(ImGuiKey_Escape))
			{
				if (IsCursorVisible())
				{
					_isMouseCursorPrevVisible = true;
				}
				else
				{
					_isMouseCursorPrevVisible = false;
					while (::ShowCursor(true) < 0);
				}
				global::mouseMode = global::MouseMode::None;
			}
		}
		viewportPos = Vector2{ pos.x, pos.y };
		viewportSize = Vector2{ maxpos.x - pos.x, maxpos.y - pos.y };

		if (myTex)
		{
			ImGui::GetWindowDrawList()->AddImage(
				myTex,
				ImVec2(pos.x, pos.y + 22),
				ImVec2(maxpos.x, maxpos.y + 22)
			);
		}

		if (!_isPlaying)
		{
			RenderGizmo(*ToolProcess::scene);
		}


	}

	ImGui::End();
}

void tool::Scene::RenderBoundingBox(core::Scene& scene)
{
	auto& registry = *scene.GetRegistry();

	_renderer->ApplyRenderState(BlendState::NO_BLEND, RasterizerState::WIREFRAME, DepthStencilState::DEPTH_ENABLED);

}

namespace
{
	struct PerObject
	{
		Matrix gWorld;
		Matrix gWorldInvTranspose;
		Matrix gTexTransform;
		Matrix gView;
		Matrix gProj;
		Matrix gViewProj;
	};

	struct LightmapInformationP
	{
		uint32_t gMa_pointShadowCount;
		bool gUseLightmap;
		bool gUseDirectionMap;
	};

	struct cbWaterMatrix
	{
		Matrix gReflectionView;
		Matrix gWindDirection;

		float gWaveLength;
		float gWaveHeight;

		float gTime;
		float gWindForce;
		float gDrawMode;

		int gFresnelMode;

		float gSpecPerturb;
		float gSpecPower;

		float gDullBlendFactor;

		int gEnableTextureBlending;
	};

	struct cbDissolveFactor
	{
		Vector3 gDissolveColor;
		float gDissolveFactor;
		float gEdgeWidth;
	};

}

void tool::Scene::RenderScene(float deltaTime, core::Scene& scene)
{
	auto& registry = *scene.GetRegistry();

	bool isRefresh = false;
	_refreshTimer += deltaTime;
	if (_refreshTimer > _refreshTime)
	{
		isRefresh = true;
		_refreshTimer -= _refreshTime;
	}


	Matrix view = _camera.ViewMatrix;
	Matrix proj = _camera.ProjectionMatrix;
	Matrix viewProj = view * proj;

	shadowPass(scene, view, proj, viewProj);

	deferredGeometryPass(scene, view, proj, viewProj);

	decalPass(scene, view, proj, viewProj);

	// Transparent
	oitPass(scene, view, proj, viewProj);


	std::unordered_map<entt::entity, std::shared_ptr<Texture>> uiTextTextures;

	{
		Texture* textures[] = { _deferredOutputTexture.get() };
		_renderer->SetRenderTargets(1, textures, nullptr, false);
		_renderer->Clear(_renderTarget->GetClearValue());

		_renderer->ApplyRenderState(BlendState::COUNT, RasterizerState::COUNT, DepthStencilState::COUNT);

		auto world = Matrix::Identity;

		auto worldInvTranspose = Matrix::Identity;

		_deferredMaterial->m_Shader->MapConstantBuffer(_renderer->GetContext());
		_deferredMaterial->m_Shader->SetMatrix("gWorld", world);
		_deferredMaterial->m_Shader->SetMatrix("gWorldInvTranspose", worldInvTranspose);
		_deferredMaterial->m_Shader->SetMatrix("gView", view);
		_deferredMaterial->m_Shader->SetMatrix("gViewProj", viewProj);
		_deferredMaterial->m_Shader->SetFloat3("gEyePosW", _camera.Position);
		_deferredMaterial->m_Shader->SetMatrix("gTexTransform", Matrix::Identity);

		_deferredMaterial->m_Shader->SetStruct("directionalLights", _directionalLights.data());
		_deferredMaterial->m_Shader->SetStruct("pointLights", _pointLights.data());
		_deferredMaterial->m_Shader->SetStruct("nonShadowPointLights", _nonShadowPointLights.data());
		_deferredMaterial->m_Shader->SetStruct("spotLights", _spotLights.data());
		_deferredMaterial->m_Shader->SetInt("numPointLights", _pointShadowCount);
		_deferredMaterial->m_Shader->SetInt("numSpotLights", _spotShadowCount);
		_deferredMaterial->m_Shader->SetFloat2("gScreenSize", { static_cast<float>(_width), static_cast<float>(_height) });

		_deferredMaterial->m_Shader->UnmapConstantBuffer(_renderer->GetContext());

		_renderer->Submit(*_quadMesh, *_deferredMaterial, 0, PrimitiveTopology::TRIANGLELIST, 1);

	}

	{
		Texture* textures[] = { _deferredOutputTexture.get() };
		// 		Texture* textures[] = { _deferredTextures[DeferredSlot::Albedo].get(),
		// 			_deferredTextures[DeferredSlot::PositionW].get(),
		// 			_deferredTextures[DeferredSlot::Emissive].get()
		// 		};

		_renderer->SetRenderTargets(_countof(textures), textures, _depthTexture.get(), false);
		std::filesystem::path path = "./Resources/Materials/DefaultSky.material";
		auto skyMaterial = _renderer->GetMaterial(path.string());
		if (skyMaterial)
		{
			auto skyShader = skyMaterial->GetShader();
			if (skyShader)
			{
				auto skyMesh = _renderer->GetMesh("Sphere");
				if (skyMesh)
				{
					_renderer->ApplyRenderState(BlendState::NO_BLEND, RasterizerState::CULL_FRONT, DepthStencilState::DEPTH_ENABLED_LESS_EQUAL);
					skyShader->MapConstantBuffer(_renderer->GetContext());
					skyShader->SetMatrix("gWorld", Matrix::CreateScale(100));
					skyShader->SetMatrix("gTexTransform", Matrix::Identity);
					skyShader->SetFloat3("gEyePosW", _camera.Position);
					skyShader->SetMatrix("gViewProj", _camera.ViewMatrix * _camera.ProjectionMatrix);
					skyShader->UnmapConstantBuffer(_renderer->GetContext());

					_renderer->Submit(*skyMesh, *skyMaterial, 0, PrimitiveTopology::TRIANGLELIST, 1);
				}
			}
		}
	}

	// Transparent Composite
	oitCompositePass(scene, view, proj, viewProj);



	reflectionPass(scene, view, proj, viewProj);


	// 합치기
	{
		Texture* textures[] = { _renderTarget.get() };
		_renderer->SetRenderTargets(1, textures, nullptr, false);
		_renderer->Clear(_renderTarget->GetClearValue());

		_renderer->ApplyRenderState(BlendState::COUNT, RasterizerState::COUNT, DepthStencilState::COUNT);

		_combineMaterial->m_Shader->MapConstantBuffer(_renderer->GetContext());
		_combineMaterial->m_Shader->UnmapConstantBuffer(_renderer->GetContext());

		_renderer->Submit(*_quadMesh, *_combineMaterial, 0, PrimitiveTopology::TRIANGLELIST, 1);


	}


	// 여기서 물 렌더 
	{
		static float elapsedTime = 0.0f;
		elapsedTime += deltaTime;
		cbWaterMatrix cbWater;
		cbWater.gDrawMode = 1.0f;
		cbWater.gFresnelMode = 0;
		cbWater.gWaveLength = 0.5f;
		cbWater.gWaveHeight = 0.05f;
		cbWater.gTime = elapsedTime;
		cbWater.gWindForce = 0.05f;
		cbWater.gSpecPerturb = 4.f;
		cbWater.gSpecPower = 364.f;
		cbWater.gDullBlendFactor = 0.2f;
		cbWater.gWindDirection = Matrix::Identity;
		cbWater.gEnableTextureBlending = 1;

		Matrix reflectionViewMatrix;
		Vector3 cameraPosition = _camera.Position;
		Vector3 targetPos = _camera.Position + _camera.GetWorldMatrix().Backward() * 1000.f;



		PerObject perObject;

		Texture* textures[] = { _renderTarget.get() };
		_renderer->SetRenderTargets(1, textures, _depthTexture.get(), false);
		_renderer->ApplyRenderState(BlendState::NO_BLEND, RasterizerState::CULL_NONE, DepthStencilState::DEPTH_ENABLED);

		for (auto&& [entity, transform, meshRenderer, renderAttributes] : registry.view<core::WorldTransform, core::MeshRenderer, core::RenderAttributes>().each())
		{
			if (!meshRenderer.isOn)
				continue;

			if (!(renderAttributes.flags & core::RenderAttributes::Flag::IsWater))
				continue;

			if (meshRenderer.mesh == nullptr)
			{
				meshRenderer.mesh = _renderer->GetMesh(meshRenderer.meshString);
				continue;
			}

			if (meshRenderer.materials.size() <= 0)
			{
				for (auto&& materialString : meshRenderer.materialStrings)
				{
					if (materialString.empty())
					{
						materialString = _defaultMaterialName;
					}
					meshRenderer.materials.push_back(_renderer->GetMaterial(materialString));
				}
				continue;
			}

			auto& world = transform.matrix;
			auto worldInvTranspose = world.Invert().Transpose();

			for (uint32_t i = 0; i < static_cast<uint32_t>(meshRenderer.materials.size()); ++i)
			{
				auto material = meshRenderer.materials[i];
				if (material == nullptr)
				{
					continue;
				}
				if (i >= meshRenderer.mesh->subMeshCount)
				{
					OutputDebugStringA("「Submesh index out of range」\n");
					continue;
				}

				perObject.gProj = proj;
				perObject.gView = view;
				perObject.gViewProj = viewProj;
				perObject.gWorld = world;
				perObject.gWorldInvTranspose = worldInvTranspose;
				perObject.gTexTransform = Matrix::Identity;

				float waterHeight = transform.position.y;
				float reflectionCamYCoord = -cameraPosition.y + 2 * waterHeight;
				Vector3 reflectionCamPosition = Vector3(cameraPosition.x, reflectionCamYCoord, cameraPosition.z);

				float reflectionTargetYCoord = -targetPos.y + 2 * waterHeight;
				Vector3 reflectionCamTarget = Vector3(targetPos.x, reflectionTargetYCoord, targetPos.z);

				Vector3 forwardVector = reflectionCamTarget - reflectionCamPosition;
				Vector3 sideVector = _camera.GetWorldMatrix().Right();
				Vector3 reflectionCamUp = DirectX::XMVector3Cross(forwardVector, sideVector);
				//DirectX::XMVector3Cross(sideVector, forwardVector)

				reflectionViewMatrix = DirectX::XMMatrixLookAtLH(reflectionCamPosition, reflectionCamTarget, reflectionCamUp);
				cbWater.gReflectionView = reflectionViewMatrix;


				material->SetTexture("RefractionMap", _deferredOutputTexture);
				material->m_Shader->MapConstantBuffer(_renderer->GetContext());
				material->m_Shader->SetConstant("cbPerObject", &perObject, sizeof(PerObject));
				material->m_Shader->SetConstant("cbWaterMatrix", &cbWater, sizeof(cbWaterMatrix));
				material->m_Shader->SetFloat3("gEyePosW", _camera.Position);
				material->m_Shader->SetInt("gRecieveDecal", meshRenderer.canReceivingDecal);
				material->m_Shader->SetInt("gReflect", false);

				material->m_Shader->SetInt("gRefract", false);

				material->m_Shader->UnmapConstantBuffer(_renderer->GetContext());

				_renderer->Submit(*meshRenderer.mesh, *material, i, PrimitiveTopology::TRIANGLELIST, 1);
			}
		}
	}



	{
		Texture* textures[] = { _pickingTexture.get() };
		_renderer->SetRenderTargets(1, textures, _depthTexture.get(), false);
		float R32ClearColor[] = { -1, -1, -1, -1 };
		_renderer->Clear(R32ClearColor);

		_renderer->ApplyRenderState(BlendState::NO_BLEND, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_ENABLED);


		for (auto&& [entity, transform, meshRenderer] : registry.view<core::WorldTransform, core::MeshRenderer>().each())
		{
			if (!meshRenderer.isOn)
				continue;

			if (meshRenderer.mesh == nullptr)
			{
				continue;
			}

			DirectX::BoundingBox boundingBox = meshRenderer.mesh->boundingBox;
			boundingBox.Transform(boundingBox, transform.matrix);
			if (_camera.frustum.Intersects(boundingBox) == DirectX::DISJOINT)
				continue;

			if (meshRenderer.materials.size() <= 0)
			{
				continue;
			}

			auto& world = transform.matrix;

			auto worldInvTranspose = world.Invert().Transpose();
			Matrix view = _camera.ViewMatrix;
			Matrix proj = _camera.ProjectionMatrix;
			Matrix viewProj = view * proj;

			for (uint32_t i = 0; i < static_cast<uint32_t>(meshRenderer.materials.size()); ++i)
			{
				auto material = _pickingMaterial;
				if (i >= meshRenderer.mesh->subMeshCount)
					continue;

				material->m_Shader->MapConstantBuffer(_renderer->GetContext());
				material->m_Shader->SetMatrix("gWorld", world);
				material->m_Shader->SetMatrix("gViewProj", viewProj);

				if (std::ranges::find(_selectedEntities, entity) != _selectedEntities.end())
				{
					material->m_Shader->SetInt("pickingColor", (int)entity);
				}
				else
				{
					material->m_Shader->SetInt("pickingColor", -1);
				}

				material->m_Shader->UnmapConstantBuffer(_renderer->GetContext());
				_renderer->Submit(*meshRenderer.mesh, *material, i, PrimitiveTopology::TRIANGLELIST, 1);
			}
		}
	}

	{
		auto&& computeShader = _outlineComputeMaterial->GetShader();
		computeShader->MapConstantBuffer(_renderer->GetContext());
		computeShader->SetFloat3("mainOutlineColor", { 1.f, 0.f, 0.f });
		computeShader->SetInt("outlineThickness", 3);
		computeShader->SetFloat3("secondOutlineColor", { 0.f, 1.f, 0.f });
		computeShader->SetInt("numSelections", static_cast<int>(_selectedEntities.size()));
		computeShader->SetInt("msaaOn", 0);

		if (!_selectedEntities.empty())
		{
			computeShader->SetConstant("SelectionListBuffer", (void*)_selectedEntities.data(), static_cast<uint32_t>(sizeof(entt::entity) * _selectedEntities.size()));
		}
		else
		{
			if (_renderer->s_Api == Renderer::API::DirectX11)
			{
				computeShader->SetInt("Selection_0", 0);
				computeShader->SetInt("Selection_1", 0);
				computeShader->SetInt("Selection_2", 0);
				computeShader->SetInt("Selection_3", 0);
				computeShader->SetInt("Selection_4", 0);
				computeShader->SetInt("Selection_5", 0);
				computeShader->SetInt("Selection_6", 0);
				computeShader->SetInt("Selection_7", 0);
				computeShader->SetInt("Selection_8", 0);
				computeShader->SetInt("Selection_9", 0);
			}
		}
		computeShader->UnmapConstantBuffer(_renderer->GetContext());
		_renderer->DispatchCompute(*_outlineComputeMaterial, _width / 16, _height / 16, 1);
	}

	{
		for (auto&& [entity, world, text] : registry.view<core::WorldTransform, core::Text>().each())
		{
			if (!text.isOn)
				continue;

			float rtClearColor[4] = { 0.0f,0.0f,0.0f,1.0f };

			if (text.font == nullptr || _textTexture == nullptr)
			{
				text.font = _renderer->GetFont(text.fontString);
				std::string entityStr = std::to_string(static_cast<uint32_t>(entity));
				_textTexture = _renderer->CreateEmptyTexture("TxtTexture", Texture::Type::Texture2D, _width, _height, 1, Texture::Format::R8G8B8A8_UNORM, Texture::Usage::RTV, rtClearColor);
				continue;
			}

			_renderer->ClearTexture(_textTexture.get(), rtClearColor);
			_renderer->SubmitText(_textTexture.get(), text.font.get(), text.text, text.position, text.size, text.scale, text.color, text.fontSize, static_cast<TextAlign>(text.textAlign), static_cast<TextBoxAlign>(text.textBoxAlign), text.useUnderline, text.useStrikeThrough);

			Texture* textures[] = { _renderTarget.get() };
			_renderer->SetRenderTargets(1, textures, _depthTexture.get(), false);
			_renderer->ApplyRenderState(BlendState::ALPHA_BLEND, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_ENABLED);

			auto&& uiShader = _uiMaterial->GetShader();

			Matrix worldMatrix = world.matrix;

			worldMatrix = Matrix::CreateScale(world.scale) * worldMatrix;
			_uiMaterial->SetTexture("gDiffuseMap", _textTexture);
			uiShader->MapConstantBuffer(_renderer->GetContext());
			uiShader->SetMatrix("gWorld", worldMatrix);
			uiShader->SetMatrix("gViewProj", _camera.ViewMatrix * _camera.ProjectionMatrix);
			uiShader->SetFloat("layerDepth", 0.0f);
			uiShader->SetFloat4("gColor", { 1.0f,1.0f,1.0f,1.0f });
			uiShader->SetInt("isOn", text.isOn);
			uiShader->SetInt("is3D", true);
			uiShader->SetFloat("maskProgress", 1);
			uiShader->SetInt("maskMode", 0);
			uiShader->UnmapConstantBuffer(_renderer->GetContext());

			_renderer->Submit(*_quadMesh, *_uiMaterial);
		}
	}

	{
		Texture* textures[] = { _renderTarget.get() };
		_renderer->SetRenderTargets(1, textures, _depthTexture.get(), false);
		_renderer->ApplyRenderState(BlendState::ALPHA_BLEND, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_ENABLED);

		auto&& uiShader = _uiMaterial->GetShader();

		for (auto&& [entity, world, common, ui] : registry.view<core::WorldTransform, core::UICommon, core::UI3D>().each())
		{
			if (common.isOn == false)
				continue;

			if (common.texture == nullptr)
			{
				common.texture = _renderer->GetUITexture(common.textureString);
				continue;
			}

			Matrix worldMatrix = world.matrix;

			if (ui.isBillboard)
			{
				switch (ui.constrainedOption)
				{
				case core::UI3D::Constrained::None:
					worldMatrix = Matrix::CreateBillboard(world.position, _camera.Position, Vector3::Up);
					break;
				case core::UI3D::Constrained::X:
					worldMatrix = Matrix::CreateConstrainedBillboard(world.position, _camera.Position, Vector3(1.0f, .0f, .0f));
					break;
				case core::UI3D::Constrained::Y:
					worldMatrix = Matrix::CreateConstrainedBillboard(world.position, _camera.Position, Vector3(.0f, 1.0f, .0f));
					break;
				case core::UI3D::Constrained::Z:
					worldMatrix = Matrix::CreateConstrainedBillboard(world.position, _camera.Position, Vector3(.0f, .0f, 1.0f));
					break;
				}

				worldMatrix = Matrix::CreateScale(world.scale) * worldMatrix;
			}

			_uiMaterial->SetTexture("gDiffuseMap", common.texture);
			uiShader->MapConstantBuffer(_renderer->GetContext());
			uiShader->SetMatrix("gWorld", worldMatrix);
			uiShader->SetMatrix("gViewProj", _camera.ViewMatrix * _camera.ProjectionMatrix);
			uiShader->SetFloat("layerDepth", 0.0f);
			uiShader->SetFloat4("gColor", common.color);
			uiShader->SetInt("isOn", common.isOn);
			uiShader->SetInt("is3D", true);
			uiShader->SetFloat("maskProgress", common.percentage);
			uiShader->SetInt("maskMode", static_cast<int>(common.maskingOption));
			uiShader->UnmapConstantBuffer(_renderer->GetContext());

			_renderer->Submit(*_quadMesh, *_uiMaterial);
		}
	}

	auto& config = ToolProcess::scene->GetRegistry()->ctx().get<core::Configuration>();

	if (_manageUI)
	{
		auto comp = [](const uint32_t& lhs, const uint32_t& rhs) {
			return lhs > rhs;
			};

		std::vector<entt::entity> bgs;
		std::map<uint32_t, std::vector<entt::entity>, std::function<bool(uint32_t, uint32_t)>> uis(comp);

		Texture* textures[] = { _renderTarget.get() };
		_renderer->SetRenderTargets(1, textures, _depthTexture.get(), false);
		_renderer->ClearTexture(_depthTexture.get(), _depthTexture->GetClearValue());

		auto&& uiShader = _uiMaterial->GetShader();

		for (auto&& [entity, common, ui] : registry.view<core::UICommon, core::UI2D>().each())
		{
			if (common.isOn == false)
				continue;

			if (common.texture == nullptr)
			{
				common.texture = _renderer->GetUITexture(common.textureString);
				continue;
			}

			if (ui.isCanvas)
				bgs.emplace_back(entity);
			else
			{
				auto findIt = uis.find(ui.layerDepth);
				if (findIt != uis.end())
					findIt->second.emplace_back(entity);
				else
				{
					std::vector<entt::entity> curUIs;
					curUIs.emplace_back(entity);
					uis[ui.layerDepth] = curUIs;
				}
			}
		}

		_renderer->ApplyRenderState(BlendState::NO_BLEND, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_ENABLED);
		for (auto& entity : bgs)
		{
			core::UICommon& common = registry.get<core::UICommon>(entity);
			core::UI2D& ui = registry.get<core::UI2D>(entity);

			_uiMaterial->SetTexture("gDiffuseMap", common.texture);
			uiShader->MapConstantBuffer(_renderer->GetContext());
			Matrix uiNDC = Matrix::Identity;

			if (ui.toolVPSize != viewportSize)
				ui.toolVPSize = viewportSize;

			uiNDC *= Matrix::CreateRotationZ(ui.rotation * DirectX::XM_PI / 180.0f);
			uiNDC *= Matrix::CreateScale(ui.size.x / ui.toolVPSize.x, ui.size.y / ui.toolVPSize.y, 1.0f);
			uiNDC *= Matrix::CreateTranslation(ui.position.x * 2 / config.width, ui.position.y * 2 / config.height, 0);

			uiShader->SetMatrix("gWorld", uiNDC);
			uiShader->SetMatrix("gViewProj", Matrix::Identity);

			uiShader->SetFloat("layerDepth", ui.layerDepth / 100.0f);
			uiShader->SetInt("is3D", false);
			uiShader->SetFloat4("gColor", common.color);
			uiShader->SetInt("isOn", common.isOn);
			uiShader->SetFloat("maskProgress", common.percentage);
			uiShader->SetInt("maskMode", static_cast<int>(common.maskingOption));
			uiShader->UnmapConstantBuffer(_renderer->GetContext());

			_renderer->Submit(*_quadMesh, *_uiMaterial);
		}

		_renderer->ApplyRenderState(BlendState::ALPHA_BLEND, RasterizerState::CULL_BACK, DepthStencilState::NO_DEPTH_WRITE);
		for (auto& vec : uis | std::views::values)
		{
			for (auto& entity : vec)
			{
				core::UICommon& common = registry.get<core::UICommon>(entity);
				core::UI2D& ui = registry.get<core::UI2D>(entity);

				_uiMaterial->SetTexture("gDiffuseMap", common.texture);
				uiShader->MapConstantBuffer(_renderer->GetContext());
				Matrix uiNDC = Matrix::Identity;

				if (ui.toolVPSize != viewportSize)
					ui.toolVPSize = viewportSize;

				uiNDC *= Matrix::CreateRotationZ(ui.rotation * DirectX::XM_PI / 180.0f);
				uiNDC *= Matrix::CreateScale(ui.size.x / ui.toolVPSize.x, ui.size.y / ui.toolVPSize.y, 1.0f);
				uiNDC *= Matrix::CreateTranslation(ui.position.x * 2 / config.width, ui.position.y * 2 / config.height, 0);

				uiShader->SetMatrix("gWorld", uiNDC);
				uiShader->SetMatrix("gViewProj", Matrix::Identity);

				uiShader->SetFloat("layerDepth", ui.layerDepth / 100.0f);
				uiShader->SetInt("is3D", false);
				uiShader->SetFloat4("gColor", common.color);
				uiShader->SetInt("isOn", common.isOn);
				uiShader->SetFloat("maskProgress", common.percentage);
				uiShader->SetInt("maskMode", static_cast<int>(common.maskingOption));
				uiShader->UnmapConstantBuffer(_renderer->GetContext());

				_renderer->Submit(*_quadMesh, *_uiMaterial);
			}
		}


		auto view = registry.view<core::Text>(entt::exclude_t<core::WorldTransform>());
		for (auto&& [entity, text] : view.each())
		{
			if (text.isOn == false)
				continue;

			if (text.font == nullptr)
			{
				text.font = _renderer->GetFont(text.fontString);
				continue;
			}

			Vector2 curRatio = { viewportSize.x * 2 / config.width,viewportSize.y * 2 / config.height };

			Vector2 toolTextPos = text.position * curRatio;
			toolTextPos += {viewportPos.x, viewportPos.y};
			toolTextPos.y += 22.0f;
			toolTextPos.x += text.leftPadding;
			toolTextPos += {viewportSize.x * 0.5f, viewportSize.y * 0.5f};

			if (core::ComboBox* comboBox = registry.try_get<core::ComboBox>(entity))
				_renderer->SubmitComboBox(_renderTarget.get(), text.font.get(), reinterpret_cast<std::vector<std::string>&>(comboBox->comboBoxTexts), comboBox->curIndex, comboBox->isOn, toolTextPos, text.leftPadding, text.size, text.scale, text.color, text.fontSize, static_cast<TextAlign>(text.textAlign), static_cast<TextBoxAlign>(text.textBoxAlign));
			else
				_renderer->SubmitText(
					_renderTarget.get(),
					text.font.get(),
					text.text,
					toolTextPos,
					text.size,
					text.scale,
					text.color,
					text.fontSize,
					static_cast<TextAlign>(text.textAlign),
					static_cast<TextBoxAlign>(text.textBoxAlign),
					text.useUnderline,
					text.useStrikeThrough);
		}
	}

	{
		float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		_renderer->SetRenderTargets(1, nullptr);
		_renderer->Clear(clearColor);

		_renderer->ApplyRenderState(BlendState::NO_BLEND, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_ENABLED);

		auto&& quadshader = _quadMaterial->GetShader();
		quadshader->MapConstantBuffer(_renderer->GetContext());
		quadshader->SetMatrix("gWorld", Matrix::Identity);
		quadshader->SetMatrix("gView", Matrix::Identity);
		quadshader->SetMatrix("gViewProj", Matrix::Identity);
		quadshader->SetMatrix("gTexTransform", Matrix::Identity);
		quadshader->UnmapConstantBuffer(_renderer->GetContext());

		_renderer->Submit(*_quadMesh, *_quadMaterial, PrimitiveTopology::TRIANGLELIST, 1);
	}

	// debug render
	{
		if (_showCollider)
		{
			// 피킹 된 애들만 렌더한다.
			_renderer->BeginDebugRender();
			_renderer->SetDebugViewProj(_camera.ViewMatrix, _camera.ProjectionMatrix);
			for (auto&& [entity, transform, boxCollider, common] : registry.view<core::WorldTransform, core::BoxCollider, core::ColliderCommon>().each())
			{
				auto finded = std::ranges::find(_selectedEntities, entity) != _selectedEntities.end();
				if (!finded)
					continue;

				DirectX::BoundingOrientedBox orientedBox;
				orientedBox.Center = common.center;
				orientedBox.Extents = boxCollider.size * 0.5f;

				orientedBox.Transform(orientedBox, transform.matrix);

				//meshRenderer.mesh->boundingBox.Transform(boundingBox, transform.matrix);
				_renderer->DrawOrientedBoundingBox(orientedBox, DirectX::XMFLOAT4(0, 1, 0, 1));

			}

			for (auto&& [entity, transform, sound] : registry.view<core::WorldTransform, core::Sound>().each())
			{
				auto finded = std::ranges::find(_selectedEntities, entity) != _selectedEntities.end();
				if (!finded)
					continue;

				if (!sound.is3D)
					continue;

				DirectX::BoundingSphere sphere;
				sphere.Center = transform.position;
				sphere.Radius = sound.maxDistance;

				sphere.Transform(sphere, transform.matrix);

				_renderer->DrawBoundingSphere(sphere, DirectX::XMFLOAT4(0, 1, 0, 1));
			}
			_renderer->EndDebugRender();
		}
	}


}

void tool::Scene::RenderGizmo(core::Scene& scene)
{
	ImGuizmo::SetOrthographic(_camera.Orthographic);    // 카메라 오소그래픽이랑 맞게
	ImGuizmo::SetDrawlist();
	ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);

	if (!_selectedEntities.empty())
	{
		auto& registry = *ToolProcess::scene->GetRegistry();

		Matrix avgMatrix = Matrix::Identity;
		avgMatrix._11 = 0.f;
		avgMatrix._22 = 0.f;
		avgMatrix._33 = 0.f;
		avgMatrix._44 = 0.f;

		int count = 0;

		for (const auto entity : _selectedEntities)
		{
			if (!registry.all_of<core::LocalTransform, core::WorldTransform>(entity))
				continue;

			auto& world = registry.get<core::WorldTransform>(entity);
			avgMatrix += world.matrix;
			count++;
		}

		if (count > 1)
		{
			avgMatrix /= static_cast<float>(count);
		}

		Matrix originalMatrix = avgMatrix;

		ImGuizmo::Manipulate(
			&_camera.ViewMatrix.m[0][0],
			&_camera.ProjectionMatrix.m[0][0],
			static_cast<ImGuizmo::OPERATION>(_currentGizmoOperation),
			static_cast<ImGuizmo::MODE>(_currentMode),
			&avgMatrix.m[0][0],
			nullptr,
			_useSnap ? &_snap.x : nullptr
		);

		if (ImGuizmo::IsUsing())
		{
			// todo 여러개 선택 및 기즈모 작동시 제대로 안됨
			Matrix deltaMatrix = avgMatrix * originalMatrix.Invert();

			for (auto entity : _selectedEntities)
			{
				if (!registry.all_of<core::WorldTransform, core::LocalTransform>(entity))
					continue;

				auto& world = registry.get<core::WorldTransform>(entity);
				auto& local = registry.get<core::LocalTransform>(entity);

				world.matrix = deltaMatrix * world.matrix;

				Vector3 scale, position; Quaternion rotation;

				bool hasParent = false;

				// 부모가 있는 경우 부모의 월드 매트릭스를 기준으로 로컬 매트릭스 계산
				if (auto* relationship = registry.try_get<core::Relationship>(entity))
				{
					if (relationship->parent != entt::null)
					{
						if (auto* parentWorld = registry.try_get<core::WorldTransform>(relationship->parent))
						{
							auto localMatrix = world.matrix * parentWorld->matrix.Invert();
							localMatrix.Decompose(scale, rotation, position);
							local.matrix = localMatrix;
							hasParent = true;
						}
					}
				}

				if (!hasParent)
				{
					world.matrix.Decompose(scale, rotation, position);
					local.matrix = world.matrix;
					registry.patch<core::LocalTransform>(entity);
					registry.patch<core::WorldTransform>(entity);
				}

				// Gizmo의 현재 동작에 따라 로컬 트랜스폼 설정
				if (_currentGizmoOperation & Move)
					local.position = position;
				if (_currentGizmoOperation & Rotate)
					local.rotation = rotation;
				if (_currentGizmoOperation & Scale)
					local.scale = scale;
			}
		}
	}
}

void tool::Scene::RenderRaytracingScene(core::Scene& scene)
{
	// 라이트와 친구들도 추가해주어야한다...

	//_renderer->FlushRaytracingData();

	auto& registry = *scene.GetRegistry();

	if (_isRendererModified)
	{
		_renderer->FlushRaytracingData();

		//처음부터 다만든다
		for (auto&& [entity, transform, meshRenderer] : registry.view<core::WorldTransform, core::MeshRenderer>().each())
		{
			if (meshRenderer.mesh == nullptr)
			{
				meshRenderer.mesh = _renderer->GetMesh(meshRenderer.meshString);
				continue;
			}

			if (meshRenderer.materials.size() <= 0)
			{
				for (auto&& materialString : meshRenderer.materialStrings)
				{
					meshRenderer.materials.push_back(_renderer->GetMaterial(materialString));
				}
				continue;
			}

			// 			if (meshRenderer.isSkinned)
			// 			{
			// // 				// 여기서 컴퓨트 셰이더 사용
			// // 				std::vector<Matrix> boneTransforms;
			// // 				boneTransforms.resize(meshRenderer.mesh->boneIndexMap.size(), Matrix::Identity);
			// // 				_computeVertexMaterial->m_Shader->SetConstant("gBoneTransforms", boneTransforms.data(), static_cast<uint32_t>(sizeof(Matrix) * boneTransforms.size()));
			// // 				_computeVertexMaterial->GetShader()->SetConstant("gVertexCount", (void*)meshRenderer.mesh->vertices.size(), sizeof(uint32_t));
			// // 				_computeVertexMaterial->SetTexture()
			// // 					_renderer->DispatchCompute();
			// 
			// 			}
			// 			else
			// 			{
			_renderer->AddMeshToScene(meshRenderer.mesh, meshRenderer.materials, &meshRenderer.hitDistribution, false);
			/*			}*/
		}

		// SBT빌드
		_renderer->AddMeshToScene(nullptr, {}, nullptr, true);

		_isRendererModified = false;
	}

	_renderer->ResetAS();

	for (auto&& [entity, transform, meshRenderer] : registry.view<core::WorldTransform, core::MeshRenderer>().each())
	{
		if (meshRenderer.mesh == nullptr)
		{
			meshRenderer.mesh = _renderer->GetMesh(meshRenderer.meshString);
			continue;
		}

		if (meshRenderer.materials.size() <= 0)
		{
			for (auto&& materialString : meshRenderer.materialStrings)
			{
				meshRenderer.materials.push_back(_renderer->GetMaterial(materialString));
			}
			continue;
		}

		_renderer->AddBottomLevelASInstance(meshRenderer.meshString, meshRenderer.hitDistribution, &transform.matrix.m[0][0]);
	}

	_renderer->UpdateAccelerationStructures();



	_renderer->SetCamera(
		_camera.ViewMatrix,
		_camera.ProjectionMatrix,
		_camera.Position,
		-_camera.GetWorldMatrix().Forward(),
		_camera.GetWorldMatrix().Up(),
		_camera.NearClipPlane,
		_camera.FarClipPlane);
	//_renderer->DrawLine(Vector3(0, 0, 0), Vector3(0, 0, 100), Color(1, 0, 0, 1));

	_renderer->DoRayTracing();

	{
		_renderer->SetViewport(_width, _height);

		float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		_renderer->SetRenderTargets(1, nullptr);
		_renderer->Clear(clearColor);

		_renderer->ApplyRenderState(BlendState::NO_BLEND, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_ENABLED);

		auto&& quadshader = _quadMaterial->GetShader();
		quadshader->MapConstantBuffer(_renderer->GetContext());
		quadshader->SetMatrix("gWorld", Matrix::Identity);
		quadshader->SetMatrix("gView", Matrix::Identity);
		quadshader->SetMatrix("gViewProj", Matrix::Identity);
		quadshader->SetMatrix("gTexTransform", Matrix::Identity);
		quadshader->UnmapConstantBuffer(_renderer->GetContext());

		_renderer->Submit(*_quadMesh, *_quadMaterial, PrimitiveTopology::TRIANGLELIST, 1);
	}


}

void tool::Scene::Resize(LONG width, LONG height)
{
	_width = width; _height = height;
	_camera.Aspect = _width / (float)_height;
	_camera.frustum.CreateFromMatrix(_camera.frustum, _camera.ProjectionMatrix);
	_camera.frustum.Transform(_camera.frustum, _camera.ViewMatrix.Invert());
	if (_renderer->IsRayTracing())
	{

	}
	else
	{
		InitializeShaderResources();
	}
}

void tool::Scene::pickItemByCPU(int mouseXPos, int mouseYPos)
{
	Matrix proj = _camera.ProjectionMatrix;

	// 프로젝션과 관련된 것 오쏘그래픽도 똑같이 되는지는 나중에 확인 필요
	float vx = (+2.0f * ((float)mouseXPos - viewportPos.x) / viewportSize.x - 1.0f) / proj._11;
	float vy = (-2.0f * ((float)mouseYPos - viewportPos.y) / viewportSize.y + 1.0f) / proj._22;

	// 뷰 스페이스의 빛 정의
	using namespace DirectX;
	using namespace DirectX::SimpleMath;
	Ray ray;
	ray.position = Vector3::Zero;
	ray.direction = Vector3(vx, vy, 1.0f);

	// 카메라의 월드로 해당 레이를 보내준다.
	Matrix view = _camera.GetWorldMatrix();

	ray.position = Vector3::Transform(ray.position, view);
	ray.direction = Vector3::TransformNormal(ray.direction, view);

	// 교차테스트를 위해 레이 길이를 정규화한다.
	ray.direction.Normalize();

	entt::entity selectedEntity = entt::null;

	float tmin = FLT_MAX;
	float t = 0.0f;

	auto&& scene = ToolProcess::scene;
	auto&& registry = *scene->GetRegistry();
	for (auto&& [entity, transform, meshRenderer] : registry.view<core::WorldTransform, core::MeshRenderer>().each())
	{
		if (meshRenderer.mesh == nullptr)
			continue;


		BoundingBox aabb = meshRenderer.mesh->boundingBox;

		aabb.Transform(aabb, transform.matrix);

		if (ray.Intersects(aabb, t))
		{
			Matrix invWorld = transform.matrix.Invert();

			for (UINT i = 0; i < meshRenderer.mesh->indices.size() / 3; ++i)
			{
				UINT i0 = meshRenderer.mesh->indices[i * 3 + 0];
				UINT i1 = meshRenderer.mesh->indices[i * 3 + 1];
				UINT i2 = meshRenderer.mesh->indices[i * 3 + 2];

				// 				Vector3 v0 = meshRenderer.mesh->vertices[i0];
				// 				Vector3 v1 = meshRenderer.mesh->vertices[i1];
				// 				Vector3 v2 = meshRenderer.mesh->vertices[i2];

				Vector3 v0 = Vector3::Transform(meshRenderer.mesh->vertices[i0], transform.matrix);
				Vector3 v1 = Vector3::Transform(meshRenderer.mesh->vertices[i1], transform.matrix);
				Vector3 v2 = Vector3::Transform(meshRenderer.mesh->vertices[i2], transform.matrix);

				if (ray.Intersects(v0, v1, v2, t))
				{
					if (t < tmin)
					{
						tmin = t;
						selectedEntity = entity;
					}
				}
			}
		}
	}

	if (tmin != FLT_MAX)
	{
		OutputDebugStringA("「Hit!」\n");
		if (ImGui::GetIO().KeyCtrl)
		{
			auto it = std::ranges::find(_selectedEntities, selectedEntity);
			if (it != _selectedEntities.end())
			{
				_selectedEntities.erase(it); // 이미 선택된 경우 선택 해제
			}
			else
			{
				_selectedEntities.push_back(selectedEntity); // 선택 목록에 추가
			}
		}
		else
		{
			_selectedEntities.clear();
			_selectedEntities.push_back(selectedEntity); // 선택 목록을 해당 엔티티로 갱신
		}
		_dispatcher->trigger<OnToolSelectEntity>({ _selectedEntities, scene });
	}
}

void tool::Scene::renderProfiler(float deltaTime)
{
	_frameCount++;
	_frameTime += deltaTime;

	if (deltaTime < _minFrameTime) _minFrameTime = deltaTime;
	if (deltaTime > _maxFrameTime) _maxFrameTime = deltaTime;

	if (_frameTime >= 1.0f)
	{
		_fps = static_cast<float>(_frameCount) / _frameTime;
		_frameCount = 0;
		_frameTime = 0.0f;
	}

	if (_isProfiling)
	{
		ImGui::Begin("Profiler");
		ImGui::Text("FPS: %.2f", _fps);
		ImGui::Text("Min Frame Time: %.3f ms", _minFrameTime * 1000.0f);
		ImGui::Text("Max Frame Time: %.3f ms", _maxFrameTime * 1000.0f);
		ImGui::End();
	}
}


void tool::Scene::selectEntity(const OnToolSelectEntity& event)
{
	_selectedEntities = event.entities;
}

void tool::Scene::playScene(const OnToolPlayScene& event)
{
	_isPlaying = true;
}

void tool::Scene::stopScene(const OnToolStopScene& event)
{
	_isPlaying = false;
	_isMouseCursorPrevVisible = true;
}

void tool::Scene::doubleClickEntity(const OnToolDoubleClickEntity& event)
{
	auto& registry = *event.scene->GetRegistry();
	auto* world = registry.try_get<core::WorldTransform>(event.entity);

	if (!world)
		return;

	Vector3 position{};
	Vector3 scale{ 1, 1, 1 };

	position = world->position;
	scale = world->scale;

	_isDoubleClicked = true;

	_scaleAvg = (std::fabs(scale.x) + std::fabs(scale.y) + std::fabs(scale.z)) / 3;

	_targetWorldPosition = position;

	_doubleClickTime = 0.f;
}

void tool::Scene::modifyRenderer()
{
	_isRendererModified = true;
}

void tool::Scene::applyCamera(const OnToolApplyCamera& event)
{
	core::Camera* camera = nullptr;
	core::WorldTransform* transform = nullptr;
	entt::entity camtity = entt::null;

	if (_isPlaying)
	{

	}
	else
	{
		auto&& registry = *ToolProcess::scene->GetRegistry();
		auto view = registry.view<core::WorldTransform, core::Camera>();

		for (auto&& [entity, tf, cam] : view.each())
		{
			if (cam.isActive)
			{
				camtity = entity;
				camera = &cam;
				transform = &tf;

				break;
			}
		}
	}

	if (camera)
	{
		if (transform)
		{
			transform->position = _camera.Position;
			transform->rotation = _camera.Rotation;
			ToolProcess::scene->GetRegistry()->patch<core::WorldTransform>(camtity);
		}

		auto& config = ToolProcess::scene->GetRegistry()->ctx().get<core::Configuration>();

		camera->aspectRatio = static_cast<float>(config.width) / config.height;
		camera->farClip = _camera.FarClipPlane;
		camera->nearClip = _camera.NearClipPlane;
		camera->fieldOfView = _camera.FieldOfView;
		camera->isOrthographic = _camera.Orthographic;
		camera->orthographicSize = _camera.size;
	}

}

void tool::Scene::initDeferredResources()
{
	float rtClearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

	const std::array<std::string, DeferredSlot::Count> textureNames = {
	"gAlbedoTexture",
	"gORMTexture",
	"gNormalTexture",
	"gPositionWTexture"
	"gEmissiveTexture",
	"gPreCalculatedLightTexture",
	"gMaskTexture"
	};

	const std::array<Texture::Format, DeferredSlot::Count> textureFormats = {
		Texture::Format::R8G8B8A8_UNORM,
		Texture::Format::R32G32B32A32_FLOAT,
		Texture::Format::R32G32B32A32_FLOAT,
		Texture::Format::R32G32B32A32_FLOAT,
		Texture::Format::R32G32B32A32_FLOAT,
		Texture::Format::R8G8B8A8_UNORM,
		Texture::Format::R32G32B32A32_FLOAT
	};

	_renderer->ReleaseTexture("SceneRT");
	_renderer->ReleaseTexture("PickingRT");
	_renderer->ReleaseTexture("MainDepthStencil");

	_deferredOutputTexture = _renderer->CreateEmptyTexture("DeferredOutput", Texture::Type::Texture2D, _width, _height, 1, Texture::Format::R8G8B8A8_UNORM, Texture::Usage::RTV, rtClearColor);

	_renderTarget = _renderer->CreateEmptyTexture("SceneRT", Texture::Type::Texture2D, _width, _height, 1, Texture::Format::R8G8B8A8_UNORM, Texture::Usage::RTV | Texture::Usage::UAV, rtClearColor);

	TextureDesc mainDSVDesc("MainDepthStencil", Texture::Type::Texture2D, _width, _height, 1, Texture::Format::R24G8_TYPELESS, Texture::Usage::DSV);
	mainDSVDesc.dsvFormat = Texture::Format::D24_UNORM_S8_UINT;
	mainDSVDesc.srvFormat = Texture::Format::R24_UNORM_X8_TYPELESS;
	_depthTexture = _renderer->CreateEmptyTexture(mainDSVDesc);

	_deferredMaterial = Material::Create(_renderer->LoadShader("./Shaders/DeferredShading.hlsl"));

	_deferredTextures.clear();
	_deferredTextures.reserve(9);
	for (int i = 0; i < DeferredSlot::Count; i++)
	{
		_renderer->ReleaseTexture(textureNames[i]);
		_deferredTextures.push_back(_renderer->CreateEmptyTexture(textureNames[i], Texture::Type::Texture2D, _width, _height, 1, textureFormats[i], Texture::Usage::RTV, rtClearColor));
	}

	_deferredMaterial->SetTexture("gAlbedo", _deferredTextures[DeferredSlot::Albedo]);
	_deferredMaterial->SetTexture("gORM", _deferredTextures[DeferredSlot::ORM]);
	_deferredMaterial->SetTexture("gNormal", _deferredTextures[DeferredSlot::Normal]);
	_deferredMaterial->SetTexture("gPositionW", _deferredTextures[DeferredSlot::PositionW]);
	_deferredMaterial->SetTexture("gEmissive", _deferredTextures[DeferredSlot::Emissive]);
	_deferredMaterial->SetTexture("gPreCalculatedLighting", _deferredTextures[DeferredSlot::PreCalculatedLight]);
	_deferredMaterial->SetTexture("gMask", _deferredTextures[DeferredSlot::Mask]);

	_deferredMaterial->SetTexture("gDirectionLightShadowMap", _dLightDepthTexture);
	_deferredMaterial->SetTexture("gSpotLightShadowMap", _sLightDepthTexture);
	_deferredMaterial->SetTexture("gPointLightShadowMap", _pLightDepthTextureRTs);

	_quadMaterial = Material::Create(_renderer->LoadShader("./Shaders/unlit.hlsl"));
	_quadMaterial->SetTexture("gDiffuseMap", _renderTarget);

	_quadMesh = _renderer->GetMesh("Quad");
	if (_quadMesh == nullptr)
	{
		auto quadMesh = MeshGenerator::CreateQuad({ -1, -1 }, { 1, 1 }, 0);

		_renderer->AddMesh(quadMesh);
		_quadMesh = _renderer->GetMesh("Quad");
	}


	// IBL
	_brdfTexture = _renderer->CreateTexture("./Resources/Textures/Default/BRDF.dds");
	_prefilteredTexture = _renderer->CreateTextureCube("./Resources/Textures/Default/DarkSkyRadiance.dds");
	_irradianceTexture = _renderer->CreateTextureCube("./Resources/Textures/Default/DarkSkyIrradiance.dds");

	_deferredMaterial->SetTexture("gIrradiance", _irradianceTexture);
	_deferredMaterial->SetTexture("gPrefilteredEnvMap", _prefilteredTexture);
	_deferredMaterial->SetTexture("gBRDFLUT", _brdfTexture);
}

void tool::Scene::finishDeferredResources()
{
	_renderer->ReleaseTexture("SceneRT");
	_renderer->ReleaseTexture("MainDepthStencil");

	// 	for (int i = 0; i < DeferredSlot::Count; i++)
	// 	{
	// 		_renderer->ReleaseTexture(_deferredTextures[i]->GetName());
	// 	}
	// 
	// 	_renderer->ReleaseTexture(_decalOutputAlbedo->GetName());
	// 	_renderer->ReleaseTexture(_brdfTexture->GetName());
	// 	_renderer->ReleaseTexture(_prefilteredTexture->GetName());
	// 	_renderer->ReleaseTexture(_irradianceTexture->GetName());

}

void tool::Scene::initLightResources()
{

	_dDepthMaterial = Material::Create(_renderer->LoadShader("./Shaders/DirectionalLightDepth.hlsl"));
	_sDepthMaterial = Material::Create(_renderer->LoadShader("./Shaders/SpotLightDepth.hlsl"));
	_pDepthMaterial = Material::Create(_renderer->LoadShader("./Shaders/PointLightDepth.hlsl"));

	_renderer->ReleaseTexture("directionalLightDepth");
	_renderer->ReleaseTexture("spotLightDepth");
	_renderer->ReleaseTexture("pointLightDepth");


	TextureDesc desc("directionalLightDepth", Texture::Type::Texture2DArray, 2048, 2048, 1, Texture::Format::R24G8_TYPELESS, Texture::Usage::DSV, nullptr, Texture::UAVType::NONE, 4);
	desc.dsvFormat = Texture::Format::D24_UNORM_S8_UINT;
	desc.srvFormat = Texture::Format::R24_UNORM_X8_TYPELESS;

	if (!_dLightDepthTexture)
		_dLightDepthTexture = _renderer->CreateEmptyTexture(desc);

	desc.name = "spotLightDepth";
	desc.width = 1024;
	desc.height = 1024;
	desc.arraySize = MAX_SHADOW_COUNT;
	if (!_sLightDepthTexture)
		_sLightDepthTexture = _renderer->CreateEmptyTexture(desc);

	desc.name = "pointLightDepth";
	desc.arraySize = MAX_POINTSHADOW_COUNT * 6;
	if (!_pLightDepthTexture)
		_pLightDepthTexture = _renderer->CreateEmptyTexture(desc);

	desc.name = "pointLightDepthRT";
	desc.format = Texture::Format::R32_FLOAT;
	desc.usage = Texture::Usage::RTV;
	desc.srvFormat = Texture::Format::R32_FLOAT;
	desc.rtvFormat = Texture::Format::R32_FLOAT;

	if (!_pLightDepthTextureRTs)
		_pLightDepthTextureRTs = _renderer->CreateEmptyTexture(desc);
}

void tool::Scene::finishLightResources()
{

}

void tool::Scene::initPostProcessResources()
{
	float decalClearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	_decalOutputAlbedo = _renderer->CreateEmptyTexture("DecalOutputAlbedo", Texture::Type::Texture2D, _width, _height,
		1, Texture::Format::R32G32B32A32_FLOAT,
		Texture::Usage::RTV | Texture::Usage::UAV, decalClearColor);

	float decalORMClearColor[] = { -1.f, -1.f, -1.f, -1.f };
	_decalOutputORM = _renderer->CreateEmptyTexture("DecalOutputORM", Texture::Type::Texture2D, _width, _height,
		1, Texture::Format::R32G32B32A32_FLOAT,
		Texture::Usage::RTV | Texture::Usage::UAV, decalORMClearColor);



	// ssr
	TextureDesc rtv32Desc(
		"ReflectionUV",
		Texture::Type::Texture2D,
		_width, _height, 1,
		Texture::Format::R32G32B32A32_FLOAT,
		Texture::Usage::RTV);

	_reflectionUVTexture = _renderer->CreateEmptyTexture(rtv32Desc);
	_reflectionUVMaterial = Material::Create(_renderer->LoadShader("./Shaders/ssreflection.hlsl"));

	_reflectionUVMaterial->SetTexture("positionTexture", _deferredTextures[DeferredSlot::PositionW]);
	_reflectionUVMaterial->SetTexture("normalTexture", _deferredTextures[DeferredSlot::Normal]);
	_reflectionUVMaterial->SetTexture("maskTexture", _deferredTextures[DeferredSlot::Mask]);


	TextureDesc rtvDesc(
		"ReflectionColor",
		Texture::Type::Texture2D,
		_width, _height, 1,
		Texture::Format::R8G8B8A8_UNORM,
		Texture::Usage::RTV);

	_reflectionColorTexture = _renderer->CreateEmptyTexture(rtvDesc);
	_reflectionColorMaterial = Material::Create(_renderer->LoadShader("./Shaders/ssreflection_color.hlsl"));

	_reflectionColorMaterial->SetTexture("uvTexture", _reflectionUVTexture);
	_reflectionColorMaterial->SetTexture("colorTexture", _deferredOutputTexture);
	_reflectionColorMaterial->SetTexture("normalTexture", _deferredTextures[DeferredSlot::Normal]);
	_reflectionColorMaterial->SetTexture("skyboxTexture", _renderer->CreateTexture("./Resources/Textures/Default/DarkSky.dds"));

	rtvDesc.name = "ReflectionBlur";
	_reflectionBlurTexture = _renderer->CreateEmptyTexture(rtvDesc);

	_reflectionBlurMaterial = Material::Create(_renderer->LoadShader("./Shaders/box_blur.hlsl"));
	_reflectionBlurMaterial->SetTexture("colorTexture", _reflectionColorTexture);

	rtvDesc.name = "ReflectionCombine";
	_reflectionCombineTexture = _renderer->CreateEmptyTexture(rtvDesc);
	_reflectionCombineMaterial = Material::Create(_renderer->LoadShader("./Shaders/reflection.hlsl"));
	_reflectionCombineMaterial->SetTexture("colorTexture", _reflectionColorTexture);
	_reflectionCombineMaterial->SetTexture("colorBlurTexture", _reflectionBlurTexture);
	_reflectionCombineMaterial->SetTexture("ormTexture", _deferredTextures[DeferredSlot::ORM]);


	_combineMaterial = Material::Create(_renderer->LoadShader("./Shaders/combine.hlsl"));
	_combineMaterial->SetTexture("originTexture", _deferredOutputTexture);
	_combineMaterial->SetTexture("reflectionTexture", _reflectionCombineTexture);



}

void tool::Scene::finishPostProcessResources()
{

}

void tool::Scene::shadowPass(core::Scene& scene, const Matrix& view, const Matrix& proj, const Matrix& viewProj)
{
	std::vector<core::DirectionalLightStructure>(MAX_SHADOW_COUNT).swap(_directionalLights);
	//std::vector<core::DirectionalLightStructure>(MAX_SHADOW_COUNT).swap(_nonShadowDirectionalLights);
	std::vector<core::PointLightStructure>(MAX_POINTSHADOW_COUNT).swap(_pointLights);
	std::vector<core::PointLightStructure>(3).swap(_nonShadowPointLights);
	std::vector<core::SpotLightStructure>(MAX_POINTSHADOW_COUNT).swap(_spotLights);
	//std::vector<core::SpotLightStructure>(MAX_SHADOW_COUNT).swap(_nonShadowSpotLights);
	/*_pointLightMap.clear();
	_pointLightEntities.clear();
	_pointLightQueue = std::queue<entt::entity>();*/

	auto& registry = *scene.GetRegistry();

	_directionalShadowCount = 0;
	_spotShadowCount = 0;
	_pointShadowCount = 0;

	for (auto&& [entity, world, lightCommon, directionalLight] : registry.view<core::WorldTransform, core::LightCommon, core::DirectionalLight>().each())
	{
		if (_directionalShadowCount >= MAX_SHADOW_COUNT)
			break;

		core::DirectionalLightStructure light;
		light.color = lightCommon.color.ToVector3();
		light.intensity = lightCommon.intensity;
		light.isOn = lightCommon.isOn;
		light.useShadow = lightCommon.useShadow;
		light.direction = Vector3::Transform(Vector3::Backward, world.rotation);
		light.lightMode = static_cast<int>(lightCommon.lightMode);
		light.direction.Normalize();

		float cascadeEnds[4] = { 0.0f, };

		core::CalculateCascadeMatrices(
			_camera.getViewMatrix(),
			_camera.ProjectionMatrix,
			_camera.FieldOfView,
			_camera.getAspect(),
			light.direction,
			_camera.getNearClipPlane(),
			_camera.getFarClipPlane(),
			4,
			light.lightViewProjection,
			cascadeEnds,
			directionalLight.cascadeEnds);

		light.cascadedEndClip = Vector4{ cascadeEnds[0], cascadeEnds[1], cascadeEnds[2], cascadeEnds[3] };

		_directionalLights[_directionalShadowCount] = light;
		_directionalShadowCount++;
	}


	for (auto&& [entity, world, lightCommon, pointLight] : registry.view<core::WorldTransform, core::LightCommon, core::PointLight>().each())
	{
		if (!lightCommon.isOn)
		{
			if (_pointLightEntities.contains(entity))
			{
				_pointLightEntities.erase(entity);
				_pointLightMap.erase(entity);

				std::queue<entt::entity> newQueue;
				while (!_pointLightQueue.empty())
				{
					auto front = _pointLightQueue.front();
					if (front != entity)
						newQueue.push(front);
					_pointLightQueue.pop();
				}

				_pointLightQueue = newQueue;
			}

			continue;
		}

		if (_pointShadowCount > MAX_POINTSHADOW_COUNT)
		{
			entt::entity eraseEntity = _pointLightQueue.front();
			core::LightCommon& erasePointLight = registry.get<core::LightCommon>(eraseEntity);
			erasePointLight.isOn = false;

			_pointLightMap.erase(eraseEntity);
			_pointLightEntities.erase(eraseEntity);
			_pointLightQueue.pop();

			core::PointLightStructure light;
			light.color = lightCommon.color.ToVector3();
			light.intensity = lightCommon.intensity;
			light.isOn = lightCommon.isOn;
			light.useShadow = lightCommon.useShadow;
			light.position = world.position;
			light.range = pointLight.range;
			light.attenuation = pointLight.attenuation;
			light.lightMode = static_cast<int>(lightCommon.lightMode);

			light.nearZ = 1.0f;

			core::CreatePointLightViewProjMatrices(
				light.position,
				light.lightViewProjection,
				light.nearZ,
				light.range);

			_pointLightMap[entity] = light;
			_pointLightEntities.insert(entity);

			break;
		}

		core::PointLightStructure light;
		light.color = lightCommon.color.ToVector3();
		light.intensity = lightCommon.intensity;
		light.isOn = lightCommon.isOn;
		light.useShadow = lightCommon.useShadow;
		light.position = world.position;
		light.range = pointLight.range;
		light.attenuation = pointLight.attenuation;
		light.lightMode = static_cast<int>(lightCommon.lightMode);

		light.nearZ = 1.0f;

		core::CreatePointLightViewProjMatrices(
			light.position,
			light.lightViewProjection,
			light.nearZ,
			light.range);

		_pointLightMap[entity] = light;
		if (!_pointLightEntities.contains(entity))
			_pointLightQueue.push(entity);

		_pointLightEntities.insert(entity);

		_pointShadowCount++;
	}

	for (auto& light : _pointLightMap | std::views::values)
		_pointLights.push_back(light);

	_pointLights.erase(_pointLights.begin(), _pointLights.begin() + MAX_POINTSHADOW_COUNT);
	_pointLights.resize(MAX_POINTSHADOW_COUNT);

	int tempIndex = 0;

	for (auto&& [entity, tag] : registry.view<core::Tag>().each())
	{
		if (tempIndex == 3)
			break;

		if (tag.id == tag::TruckLight::id)
		{
			auto& truckLight = registry.get<core::PointLight>(entity);
			auto& world = registry.get<core::WorldTransform>(entity);
			auto& lightCommon = registry.get<core::LightCommon>(entity);

			core::PointLightStructure light;
			light.color = lightCommon.color.ToVector3();
			light.intensity = lightCommon.intensity;
			light.isOn = lightCommon.isOn;
			light.useShadow = false;
			light.position = world.position;
			light.range = truckLight.range;
			light.attenuation = truckLight.attenuation;
			light.lightMode = static_cast<int>(lightCommon.lightMode);

			_nonShadowPointLights.push_back(light);

			tempIndex++;
		}
	}

	for (auto&& [entity, world, lightCommon, spotLight] : registry.view<core::WorldTransform, core::LightCommon, core::SpotLight>().each())
	{
		if (_spotShadowCount >= MAX_SHADOW_COUNT)
			break;

		core::SpotLightStructure light;
		light.color = lightCommon.color.ToVector3();
		light.intensity = lightCommon.intensity;
		light.isOn = lightCommon.isOn;
		light.useShadow = lightCommon.useShadow;
		light.position = world.position;
		light.range = spotLight.range;
		light.direction = Vector3::Transform(Vector3::Backward, world.rotation);
		light.direction.Normalize();
		light.attenuation = spotLight.attenuation;
		light.spot = spotLight.angularAttenuationIndex;
		light.lightMode = static_cast<int>(lightCommon.lightMode);

		light.spotAngle = spotLight.spotAngle;
		light.innerAngle = spotLight.innerAngle;

		auto focusPosition = light.position + light.direction * light.range;

		Matrix lightView = DirectX::XMMatrixLookAtLH(light.position, focusPosition, Vector3::Up);

		auto spotAngleRadians = light.spotAngle * (DirectX::XM_PI / 180.0f);
		float nearPlane = light.range * 0.1f;
		Matrix lightProj = DirectX::XMMatrixPerspectiveFovLH(spotAngleRadians, 1.0f, nearPlane, light.range);
		light.lightViewProjection = lightView * lightProj;

		_spotLights[_spotShadowCount] = light;
		_spotShadowCount++;
	}


	PerObject perObject;
	{
		_renderer->SetViewport(2048, 2048);
		_renderer->SetRenderTargets(0, nullptr, _dLightDepthTexture.get(), false);
		_renderer->Clear(_dLightDepthTexture->GetClearValue());

		for (int dLightIndex = 0; dLightIndex < _directionalShadowCount; dLightIndex++)
		{
			if (!_directionalLights[dLightIndex].isOn || !_directionalLights[dLightIndex].useShadow)
				continue;

			for (auto&& [entity, transform, meshRenderer] : registry.view<core::WorldTransform, core::MeshRenderer>().each())
			{
				float distance = (transform.position - _camera.Position).Length();

				if (!meshRenderer.isOn || !meshRenderer.receiveShadow || distance >= 100.0f)
					continue;

				if (meshRenderer.mesh == nullptr)
				{
					meshRenderer.mesh = _renderer->GetMesh(meshRenderer.meshString);
					continue;
				}

				if (!meshRenderer.isCulling)
					_renderer->ApplyRenderState(BlendState::NO_BLEND, RasterizerState::SHADOW_CULL_NONE, DepthStencilState::DEPTH_ENABLED);
				else
					_renderer->ApplyRenderState(BlendState::NO_BLEND, RasterizerState::SHADOW, DepthStencilState::DEPTH_ENABLED);

				auto& world = transform.matrix;

				auto worldInvTranspose = world.Invert().Transpose();

				perObject.gProj = proj;
				perObject.gView = view;
				perObject.gViewProj = viewProj;
				perObject.gWorld = world;
				perObject.gWorldInvTranspose = worldInvTranspose;
				perObject.gTexTransform = Matrix::Identity;

				for (uint32_t i = 0; i < meshRenderer.mesh->subMeshCount; i++)
				{
					_dDepthMaterial->m_Shader->MapConstantBuffer(_renderer->GetContext());
					_dDepthMaterial->m_Shader->SetConstant("cbPerObject", &perObject, sizeof(PerObject));
					_dDepthMaterial->m_Shader->SetStruct("directionalLights", _directionalLights.data());
					_dDepthMaterial->m_Shader->SetInt("useAlphaMap", false);

					if (meshRenderer.materials.size() > i)
					{
						if (meshRenderer.materials[i] != nullptr)
						{
							auto iter = meshRenderer.materials[i]->GetTextures().find("gAlpha");

							if (iter != meshRenderer.materials[i]->GetTextures().end())
							{
								_dDepthMaterial->m_Shader->SetInt("useAlphaMap", true);
								_dDepthMaterial->SetTexture("gAlpha", iter->second.Texture);
							}
						}
					}

					_dDepthMaterial->m_Shader->UnmapConstantBuffer(_renderer->GetContext());
					_renderer->Submit(*meshRenderer.mesh, *_dDepthMaterial, i, PrimitiveTopology::TRIANGLELIST, 1);
				}
			}
		}

		Texture* plTextures[1] =
		{
			_pLightDepthTextureRTs.get()
		};
		_renderer->SetViewport(1024, 1024);

		_renderer->SetRenderTargets(1, plTextures, _pLightDepthTexture.get(), false);
		_renderer->Clear(_pLightDepthTexture->GetClearValue());

		for (int pLightIndex = 0; pLightIndex < _pointShadowCount; pLightIndex++)
		{
			if (!_pointLights[pLightIndex].isOn || !_pointLights[pLightIndex].useShadow)
				continue;

			for (auto&& [entity, transform, meshRenderer] : registry.view<core::WorldTransform, core::MeshRenderer>().each())
			{
				if (!meshRenderer.isOn || !meshRenderer.receiveShadow)
					continue;

				if (meshRenderer.mesh == nullptr)
				{
					meshRenderer.mesh = _renderer->GetMesh(meshRenderer.meshString);
					continue;
				}

				auto&& meshPosition = transform.position;
				auto&& lightPosition = _pointLights[pLightIndex].position;

				auto distance = Vector3::Distance(meshPosition, lightPosition);
				if (distance > _pointLights[pLightIndex].range)
					continue;

				if (!meshRenderer.isCulling)
					_renderer->ApplyRenderState(BlendState::NO_BLEND, RasterizerState::PSHADOW_CULL_NONE, DepthStencilState::DEPTH_ENABLED);
				else
					_renderer->ApplyRenderState(BlendState::NO_BLEND, RasterizerState::PSHADOW, DepthStencilState::DEPTH_ENABLED);

				auto& world = transform.matrix;
				auto worldInvTranspose = world.Invert().Transpose();

				perObject.gProj = proj;
				perObject.gView = view;
				perObject.gViewProj = viewProj;
				perObject.gWorld = world;
				perObject.gWorldInvTranspose = worldInvTranspose;
				perObject.gTexTransform = Matrix::Identity;

				_pDepthMaterial->m_Shader->MapConstantBuffer(_renderer->GetContext());
				_pDepthMaterial->m_Shader->SetConstant("cbPerObject", &perObject, sizeof(PerObject));
				_pDepthMaterial->m_Shader->SetStruct("pointLights", _pointLights.data());
				_pDepthMaterial->m_Shader->SetInt("numPointLights", _pointShadowCount);
				_pDepthMaterial->m_Shader->UnmapConstantBuffer(_renderer->GetContext());

				for (uint32_t i = 0; i < meshRenderer.mesh->subMeshCount; i++)
					_renderer->Submit(*meshRenderer.mesh, *_pDepthMaterial, i, PrimitiveTopology::TRIANGLELIST, 1);
			}
		}


		_renderer->SetViewport(1024, 1024);
		_renderer->SetRenderTargets(0, nullptr, _sLightDepthTexture.get(), false);
		_renderer->Clear(_renderTarget->GetClearValue());

		for (int sLightIndex = 0; sLightIndex < _spotShadowCount; sLightIndex++)
		{
			if (!_spotLights[sLightIndex].isOn || !_spotLights[sLightIndex].useShadow)
				continue;

			for (auto&& [entity, transform, meshRenderer] : registry.view<core::WorldTransform, core::MeshRenderer>().each())
			{
				if (!meshRenderer.isOn || !meshRenderer.receiveShadow)
					continue;

				if (meshRenderer.mesh == nullptr)
				{
					meshRenderer.mesh = _renderer->GetMesh(meshRenderer.meshString);
					continue;
				}

				auto meshPosition = transform.position;
				auto lightPosition = _spotLights[sLightIndex].position;

				auto distance = Vector3::Distance(meshPosition, lightPosition);
				if (distance > _spotLights[sLightIndex].range)
					continue;

				if (!meshRenderer.isCulling)
					_renderer->ApplyRenderState(BlendState::NO_BLEND, RasterizerState::SHADOW_CULL_NONE, DepthStencilState::DEPTH_ENABLED);
				else
					_renderer->ApplyRenderState(BlendState::NO_BLEND, RasterizerState::SHADOW, DepthStencilState::DEPTH_ENABLED);

				auto& world = transform.matrix;

				auto worldInvTranspose = world.Invert().Transpose();

				perObject.gProj = proj;
				perObject.gView = view;
				perObject.gViewProj = viewProj;
				perObject.gWorld = world;
				perObject.gWorldInvTranspose = worldInvTranspose;
				perObject.gTexTransform = Matrix::Identity;

				_sDepthMaterial->m_Shader->MapConstantBuffer(_renderer->GetContext());
				_sDepthMaterial->m_Shader->SetConstant("cbPerObject", &perObject, sizeof(PerObject));
				_sDepthMaterial->m_Shader->SetStruct("spotLights", _spotLights.data());
				_sDepthMaterial->m_Shader->SetInt("numSpotLights", static_cast<int>(_spotLights.size()));
				_sDepthMaterial->m_Shader->UnmapConstantBuffer(_renderer->GetContext());

				for (uint32_t i = 0; i < meshRenderer.mesh->subMeshCount; i++)
					_renderer->Submit(*meshRenderer.mesh, *_sDepthMaterial, i, PrimitiveTopology::TRIANGLELIST, 1);
			}
		}
	}
}

void tool::Scene::deferredGeometryPass(core::Scene& scene, const Matrix& view, const Matrix& proj, const Matrix& viewProj)
{
	auto& registry = *scene.GetRegistry();
	PerObject perObject;
	cbDissolveFactor dissolveFactor;

	dissolveFactor.gDissolveColor = Vector3(1.0f, 0.0f, 0.0f);
	dissolveFactor.gDissolveFactor = 0.0f;
	dissolveFactor.gEdgeWidth = -0.001f;

	{
		Texture* textures[DeferredSlot::Count] =
		{
			_deferredTextures[DeferredSlot::Albedo].get(),
			_deferredTextures[DeferredSlot::ORM].get(),
			_deferredTextures[DeferredSlot::Normal].get(),
			_deferredTextures[DeferredSlot::PositionW].get(),
			_deferredTextures[DeferredSlot::Emissive].get(),
			_deferredTextures[DeferredSlot::PreCalculatedLight].get(),
			_deferredTextures[DeferredSlot::Mask].get()
		};
		_renderer->SetViewport(_width, _height);
		_renderer->SetRenderTargets(DeferredSlot::Count, textures, _depthTexture.get(), false);
		_renderer->Clear(_renderTarget->GetClearValue());

		for (auto&& [entity, transform, meshRenderer] : registry.view<core::WorldTransform, core::MeshRenderer>().each())
		{

			if (!meshRenderer.isOn)
				continue;

			if (meshRenderer.mesh == nullptr)
			{
				meshRenderer.mesh = _renderer->GetMesh(meshRenderer.meshString);
				continue;
			}

			auto* renderAttributes = registry.try_get<core::RenderAttributes>(entity);

			if (renderAttributes && renderAttributes->flags & core::RenderAttributes::Flag::IsTransparent
				|| renderAttributes && renderAttributes->flags & core::RenderAttributes::Flag::IsWater)
				continue;

			DirectX::BoundingBox boundingBox = meshRenderer.mesh->boundingBox;
			boundingBox.Transform(boundingBox, transform.matrix);
			if (_camera.frustum.Intersects(boundingBox) == DirectX::DISJOINT && !meshRenderer.isSkinned)
				continue;

			if (!meshRenderer.isCulling)
				_renderer->ApplyRenderState(BlendState::COUNT, RasterizerState::CULL_NONE, DepthStencilState::DEPTH_ENABLED);
			else
				_renderer->ApplyRenderState(BlendState::COUNT, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_ENABLED);


			if (meshRenderer.materials.size() <= 0)
			{
				for (auto&& materialString : meshRenderer.materialStrings)
				{
					if (materialString.empty())
					{
						materialString = _defaultMaterialName;
					}
					meshRenderer.materials.push_back(_renderer->GetMaterial(materialString));
				}
				continue;
			}

			auto& world = transform.matrix;
			auto worldInvTranspose = world.Invert().Transpose();

			for (uint32_t i = 0; i < static_cast<uint32_t>(meshRenderer.materials.size()); ++i)
			{
				auto material = meshRenderer.materials[i];
				if (material == nullptr)
				{
					continue;
				}
				if (i >= meshRenderer.mesh->subMeshCount)
				{
					OutputDebugStringA("「Submesh index out of range」\n");
					continue;
				}

				perObject.gProj = proj;
				perObject.gView = view;
				perObject.gViewProj = viewProj;
				perObject.gWorld = world;
				perObject.gWorldInvTranspose = worldInvTranspose;
				perObject.gTexTransform = Matrix::Identity;

				material->m_Shader->MapConstantBuffer(_renderer->GetContext());
				material->m_Shader->SetConstant("cbPerObject", &perObject, sizeof(PerObject));
				material->m_Shader->SetFloat3("gEyePosW", _camera.Position);
				material->m_Shader->SetInt("gRecieveDecal", meshRenderer.canReceivingDecal);
				material->m_Shader->SetInt("gRefract", false);
				material->m_Shader->SetConstant("cbDissolveFactor", &dissolveFactor, sizeof(cbDissolveFactor));

				if (renderAttributes)
				{
					material->m_Shader->SetInt("gReflect", renderAttributes->flags & core::RenderAttributes::Flag::CanReflect);
				}
				else
				{
					material->m_Shader->SetInt("gReflect", false);
				}

				material->m_Shader->SetFloat("gEmissiveFactor", meshRenderer.emissiveFactor);
				if (meshRenderer.isSkinned)
				{
					SubMeshDescriptor& subMesh = meshRenderer.mesh->subMeshDescriptors[i];
					std::vector<Matrix> boneTransforms;
					boneTransforms.resize(subMesh.boneIndexMap.size(), Matrix::Identity);
					material->m_Shader->SetConstant("BoneMatrixBuffer", boneTransforms.data(), static_cast<uint32_t>(sizeof(Matrix) * boneTransforms.size()));
				}

				material->m_Shader->UnmapConstantBuffer(_renderer->GetContext());

				_renderer->Submit(*meshRenderer.mesh, *material, i, PrimitiveTopology::TRIANGLELIST, 1);
			}
		}
	}

}

void tool::Scene::initOITResources()
{
	float accClearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float rvlClearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	_revealageTexture = _renderer->CreateEmptyTexture("RevealageTexture", Texture::Type::Texture2D, _width, _height, 1, Texture::Format::R8G8B8A8_UNORM, Texture::Usage::RTV | Texture::Usage::UAV, rvlClearColor);
	_accumulationTexture = _renderer->CreateEmptyTexture("AccumulationTexture", Texture::Type::Texture2D, _width, _height, 1, Texture::Format::R32G32B32A32_FLOAT, Texture::Usage::RTV | Texture::Usage::UAV, accClearColor);

	_oitCombineMaterial = Material::Create(_renderer->LoadShader("./Shaders/TransparencyComposite.hlsl"));
	_oitCombineMaterial->SetTexture("gReveal", _revealageTexture);
	_oitCombineMaterial->SetTexture("gAccum", _accumulationTexture);
}

void tool::Scene::oitPass(core::Scene& scene, const Matrix& view, const Matrix& proj, const Matrix& viewProj)
{
	auto& registry = *scene.GetRegistry();
	PerObject perObject;

	{
		Texture* textures[] = { _revealageTexture.get(), _accumulationTexture.get() };
		_renderer->SetRenderTargets(2, textures, _depthTexture.get(), false);
		_renderer->ClearTexture(_revealageTexture.get(), _revealageTexture->GetClearValue());
		_renderer->ClearTexture(_accumulationTexture.get(), _accumulationTexture->GetClearValue());
		_renderer->ApplyRenderState(BlendState::TRANSPARENT_BLEND, RasterizerState::CULL_NONE, DepthStencilState::DEPTH_ENABLED);

		for (auto&& [entity, transform, meshRenderer, renderAttributes] : registry.view<core::WorldTransform, core::MeshRenderer, core::RenderAttributes>().each())
		{
			if (!meshRenderer.isOn)
				continue;

			if (!(renderAttributes.flags & core::RenderAttributes::Flag::IsTransparent)
				|| renderAttributes.flags & core::RenderAttributes::Flag::IsWater)
				continue;

			if (meshRenderer.mesh == nullptr)
			{
				meshRenderer.mesh = _renderer->GetMesh(meshRenderer.meshString);
				continue;
			}

			if (meshRenderer.materials.size() <= 0)
			{
				for (auto&& materialString : meshRenderer.materialStrings)
				{
					if (materialString.empty())
					{
						materialString = _defaultMaterialName;
					}
					meshRenderer.materials.push_back(_renderer->GetMaterial(materialString));
				}
				continue;
			}

			auto& world = transform.matrix;
			auto worldInvTranspose = world.Invert().Transpose();

			for (uint32_t i = 0; i < static_cast<uint32_t>(meshRenderer.materials.size()); ++i)
			{
				auto material = meshRenderer.materials[i];
				if (material == nullptr)
				{
					continue;
				}
				if (i >= meshRenderer.mesh->subMeshCount)
				{
					OutputDebugStringA("「Submesh index out of range」\n");
					continue;
				}

				perObject.gProj = proj;
				perObject.gView = view;
				perObject.gViewProj = viewProj;
				perObject.gWorld = world;
				perObject.gWorldInvTranspose = worldInvTranspose;
				perObject.gTexTransform = Matrix::Identity;

				material->m_Shader->MapConstantBuffer(_renderer->GetContext());
				material->m_Shader->SetConstant("cbPerObject", &perObject, sizeof(PerObject));
				material->m_Shader->SetFloat3("gEyePosW", _camera.Position);
				material->m_Shader->SetInt("gRecieveDecal", meshRenderer.canReceivingDecal);
				material->m_Shader->SetInt("gReflect", false);

				material->m_Shader->SetInt("gRefract", false);

				material->m_Shader->UnmapConstantBuffer(_renderer->GetContext());

				_renderer->Submit(*meshRenderer.mesh, *material, i, PrimitiveTopology::TRIANGLELIST, 1);
			}
		}
	}
}

void tool::Scene::oitCompositePass(core::Scene& scene, const Matrix& view, const Matrix& proj, const Matrix& viewProj)
{
	{
		Texture* textures[] = { _deferredOutputTexture.get() };
		_renderer->SetRenderTargets(1, textures, nullptr, false);
		_renderer->ApplyRenderState(BlendState::ALPHA_BLEND, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_ENABLED);

		auto world = Matrix::Identity;
		auto worldInvTranspose = Matrix::Identity;

		_oitCombineMaterial->m_Shader->MapConstantBuffer(_renderer->GetContext());
		_oitCombineMaterial->m_Shader->SetMatrix("gWorld", world);
		_oitCombineMaterial->m_Shader->SetMatrix("gWorldInvTranspose", worldInvTranspose);
		_oitCombineMaterial->m_Shader->SetMatrix("gView", view);
		_oitCombineMaterial->m_Shader->SetMatrix("gViewProj", viewProj);
		_oitCombineMaterial->m_Shader->SetFloat3("gEyePosW", _camera.Position);
		_oitCombineMaterial->m_Shader->SetMatrix("gTexTransform", Matrix::Identity);
		_oitCombineMaterial->m_Shader->UnmapConstantBuffer(_renderer->GetContext());


		_renderer->Submit(*_quadMesh, *_oitCombineMaterial, 0, PrimitiveTopology::TRIANGLELIST, 1);
	}


}

void tool::Scene::decalPass(core::Scene& scene, const Matrix& view, const Matrix& proj, const Matrix& viewProj)
{

	auto& registry = *scene.GetRegistry();

	/// 여기에 데칼 코드 추가
	{
		auto viewInveres = view.Invert();
		_renderer->ClearTexture(_decalOutputAlbedo.get(), _decalOutputAlbedo->GetClearValue());
		_renderer->ClearTexture(_decalOutputORM.get(), _decalOutputORM->GetClearValue());

		for (auto&& [entity, transform, decal] : registry.view<core::WorldTransform, core::Decal>().each())
		{
			if (decal.material == nullptr)
			{
				if (!decal.materialString.empty())
				{
					decal.material = _renderer->GetMaterial(decal.materialString);
				}
				continue;
			}

			decal.material->SetTexture("gNormalTexture", _deferredTextures[DeferredSlot::Normal]);
			decal.material->SetTexture("gDepthTexture", _deferredTextures[DeferredSlot::PositionW]);
			decal.material->SetUAVTexture("OutputTexture", _decalOutputAlbedo);
			decal.material->SetUAVTexture("OrmOutputTexture", _decalOutputORM);

			auto& world = transform.matrix;
			auto worldInverse = world.Invert();
			auto WVP = world * view * proj;

			decal.material->m_Shader->MapConstantBuffer(_renderer->GetContext());
			decal.material->m_Shader->SetMatrix("gWVP", WVP);
			decal.material->m_Shader->SetMatrix("gWorldInv", worldInverse);
			decal.material->m_Shader->SetMatrix("gViewInv", viewInveres);
			decal.material->m_Shader->SetFloat("fTargetWidth", (float)_width);
			decal.material->m_Shader->SetFloat("fTargetHeight", (float)_height);
			decal.material->m_Shader->SetFloat("gFadeFactor", decal.fadeFactor);
			decal.material->m_Shader->SetFloat3("gDecalDirection", transform.matrix.Up());
			decal.material->m_Shader->UnmapConstantBuffer(_renderer->GetContext());

			_renderer->DispatchCompute(*decal.material, _width / 16, _height / 16, 1);
		}
	}
}

void tool::Scene::reflectionPass(core::Scene& scene, const Matrix& view, const Matrix& proj, const Matrix& viewProj)
{
	/// 여기에 반사 코드 추가
	//if (isRefresh)
	{
		{
			Texture* textures[] = { _reflectionUVTexture.get() };
			_renderer->SetRenderTargets(1, textures, nullptr, false);
			_renderer->Clear(_renderTarget->GetClearValue());

			_renderer->ApplyRenderState(BlendState::COUNT, RasterizerState::COUNT, DepthStencilState::COUNT);

			Matrix proj = _camera.ProjectionMatrix;

			_reflectionUVMaterial->m_Shader->MapConstantBuffer(_renderer->GetContext());
			_reflectionUVMaterial->m_Shader->SetMatrix("lensProjection", proj);
			_reflectionUVMaterial->m_Shader->SetMatrix("lensView", _camera.ViewMatrix);
			_reflectionUVMaterial->m_Shader->SetFloat2("texSize", { (float)_width, (float)_height });
			_reflectionUVMaterial->m_Shader->SetFloat2("enabled", { 1.0f, 0.0f });
			_reflectionUVMaterial->m_Shader->UnmapConstantBuffer(_renderer->GetContext());

			_renderer->Submit(*_quadMesh, *_reflectionUVMaterial, 0, PrimitiveTopology::TRIANGLELIST, 1);
		}

		/// 반사 색상
		{
			Texture* textures[] = { _reflectionColorTexture.get() };
			_renderer->SetRenderTargets(1, textures, nullptr, false);
			_renderer->Clear(_renderTarget->GetClearValue());

			_renderer->ApplyRenderState(BlendState::COUNT, RasterizerState::COUNT, DepthStencilState::COUNT);

			_reflectionColorMaterial->m_Shader->MapConstantBuffer(_renderer->GetContext());
			_reflectionColorMaterial->m_Shader->SetFloat2("texSize", { (float)_width, (float)_height });
			_reflectionColorMaterial->m_Shader->SetFloat3("viewPosition", _camera.Position);
			_reflectionColorMaterial->m_Shader->UnmapConstantBuffer(_renderer->GetContext());

			_renderer->Submit(*_quadMesh, *_reflectionColorMaterial, 0, PrimitiveTopology::TRIANGLELIST, 1);
		}

		/// 반사 블러링
		{
			Texture* textures[] = { _reflectionBlurTexture.get() };
			_renderer->SetRenderTargets(1, textures, nullptr, false);
			_renderer->Clear(_renderTarget->GetClearValue());

			_renderer->ApplyRenderState(BlendState::COUNT, RasterizerState::COUNT, DepthStencilState::COUNT);

			_reflectionBlurMaterial->m_Shader->MapConstantBuffer(_renderer->GetContext());
			_reflectionBlurMaterial->m_Shader->SetFloat2("parameters", { (float)5, (float)5 });
			_reflectionBlurMaterial->m_Shader->UnmapConstantBuffer(_renderer->GetContext());

			_renderer->Submit(*_quadMesh, *_reflectionBlurMaterial, 0, PrimitiveTopology::TRIANGLELIST, 1);
		}

		/// 반사 합체
		{
			Texture* textures[] = { _reflectionCombineTexture.get() };
			_renderer->SetRenderTargets(1, textures, nullptr, false);
			_renderer->Clear(_renderTarget->GetClearValue());

			_renderer->ApplyRenderState(BlendState::COUNT, RasterizerState::COUNT, DepthStencilState::COUNT);

			_reflectionCombineMaterial->m_Shader->MapConstantBuffer(_renderer->GetContext());
			_reflectionCombineMaterial->m_Shader->UnmapConstantBuffer(_renderer->GetContext());

			_renderer->Submit(*_quadMesh, *_reflectionCombineMaterial, 0, PrimitiveTopology::TRIANGLELIST, 1);

		}
	}

}

void tool::Camera::SetLens()
{
	/// 프로젝션 행렬 만드는 핵심함수
	if (OrthographicValue == true)
	{
		ProjectionMatrix = DirectX::XMMatrixOrthographicLH(
			size * AspectValue, size,
			NearClipPlaneValue,
			FarClipPlaneValue);
	}
	else
	{
		ProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(FieldOfViewValue, AspectValue, NearClipPlaneValue, FarClipPlaneValue);
	}
}

void tool::Camera::CameraMove(float deltaTime)
{
	Vector3 forw = -m_WorldMatrix.Forward();
	Vector3 right = m_WorldMatrix.Right();
	Vector3 up = m_WorldMatrix.Up();

	if (ImGui::IsKeyDown(ImGuiKey_W))
		Position += forw * Speed * deltaTime;

	if (ImGui::IsKeyDown(ImGuiKey_S))
		Position -= forw * Speed * deltaTime;

	if (ImGui::IsKeyDown(ImGuiKey_A))
		Position -= right * Speed * deltaTime;

	if (ImGui::IsKeyDown(ImGuiKey_D))
		Position += right * Speed * deltaTime;

	if (ImGui::IsKeyDown(ImGuiKey_Q))
		Position -= up * Speed * deltaTime;

	if (ImGui::IsKeyDown(ImGuiKey_E))
		Position += up * Speed * deltaTime;

	if (ImGui::IsKeyDown(ImGuiKey_Z))
		size += 1 * deltaTime;

	if (ImGui::IsKeyDown(ImGuiKey_X))
		size -= 1 * deltaTime;
}

void tool::Camera::CameraDrag(float deltaTime)
{
	Vector3 forward = -m_WorldMatrix.Forward();
	Vector3 right = m_WorldMatrix.Right();
	Vector3 up = m_WorldMatrix.Up();

	Vector2 mousePos = { ImGui::GetMousePos().x, ImGui::GetMousePos().y };
	Vector2 delta = mousePos - m_PrevCur;

	Position += right * delta.x * Speed * deltaTime;
	Position += up * -delta.y * Speed * deltaTime;
}

void tool::Camera::CameraMoveRotate(float deltaTime)
{
	// 회전
	Vector2 mousePos = { ImGui::GetMousePos().x, ImGui::GetMousePos().y };

	Quaternion r = Rotation;

	if (ImGui::IsKeyDown(ImGuiKey_MouseRight))
	{
		CameraMove(deltaTime);

		Vector2 offset = { mousePos.x - m_PrevCur.x, mousePos.y - m_PrevCur.y };

		if (offset.x > 0 || offset.x < 0 || offset.y < 0 || offset.y > 0)
		{
			//yaw
			Quaternion r2 = Quaternion::CreateFromAxisAngle({ 0,1,0 }, offset.x * 0.005f);

			//pitch
			Quaternion r3 = Quaternion::CreateFromAxisAngle({ 1,0,0 }, offset.y * 0.005f);

			Quaternion r4 = r3 * r;

			//cameraOrientation * frameYaw
			Quaternion r5 = r4 * r2;

			r = r5;
		}
	}

	Rotation = r;

	// update frustum
	// 현재 툴에서 카메라 움직임은 이것만 하고 있어서 여기서만 업데이트
	frustum.CreateFromMatrix(frustum, ProjectionMatrix);
	frustum.Transform(frustum, ViewMatrix.Invert());
}

