#include "pch.h"
#include "PreRenderSystem.h"

#include "Scene.h"
#include "RenderComponents.h"
#include "CoreComponents.h"

#include "../Animavision/Renderer.h"
#include "../Animavision/ShaderResource.h"
#include "../Animavision/Material.h"
#include "../Animavision/Shader.h"
#include "../Animavision/Mesh.h"


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
}

core::PreRenderSystem::PreRenderSystem(Scene& scene)
	: ISystem(scene)
{
	_dispatcher = scene.GetDispatcher();
	_dispatcher->sink<OnPreStartSystem>().connect<&PreRenderSystem::preStartSystem>(this);
	_dispatcher->sink<OnStartSystem>().connect<&PreRenderSystem::startSystem>(this);
	_dispatcher->sink<OnFinishSystem>().connect<&PreRenderSystem::finishSystem>(this);

	_dispatcher->sink<OnDestroyRenderResources>().connect<&PreRenderSystem::destroyRenderResources>(this);
	_dispatcher->sink<OnCreateRenderResources>().connect<&PreRenderSystem::createRenderResources>(this);

}

core::PreRenderSystem::~PreRenderSystem()
{
	_dispatcher->disconnect(this);

}

void core::PreRenderSystem::PreRender(Scene& scene, Renderer& renderer, float tick)
{
	// Camera 가져오기
	auto& registry = *scene.GetRegistry();
	{
		auto view = registry.view<core::Camera, core::WorldTransform>();
		for (auto&& [entity, camera, transform] : view.each())
		{
			if (camera.isActive)
			{
				_mainCamera = &camera;
				_mainCameraTransform = &transform;
				break;
			}
		}
	}

	Matrix view = Matrix::Identity;

	if (_mainCameraTransform)
		view = _mainCameraTransform->matrix.Invert();

	Matrix proj = Matrix::Identity;

	if (_mainCamera)
	{
		auto& mainCamera = *_mainCamera;
		mainCamera.aspectRatio = static_cast<float>(_renderResources->width) / static_cast<float>(_renderResources->height);

		if (_mainCamera->isOrthographic)
		{
			proj = DirectX::XMMatrixOrthographicLH(
				mainCamera.orthographicSize * mainCamera.aspectRatio,
				mainCamera.orthographicSize,
				mainCamera.nearClip,
				mainCamera.farClip);
		} 
		else
		{
			proj = DirectX::XMMatrixPerspectiveFovLH(
				mainCamera.fieldOfView,
				mainCamera.aspectRatio,
				mainCamera.nearClip,
				mainCamera.farClip);
		}
	}

	Matrix viewProj = view * proj;

	auto& renderRes = scene.GetRegistry()->ctx().get<core::RenderResources>();
	renderRes.viewMatrix = view;
	renderRes.projectionMatrix = proj;
	renderRes.viewProjectionMatrix = viewProj;
	renderRes.cameraTransform = _mainCameraTransform;
	renderRes.mainCamera = _mainCamera;


	auto deferredTextures = _renderResources->deferredTextures;

	Texture* textures[] = {
		deferredTextures[DeferredSlot::Albedo].get(),
		deferredTextures[DeferredSlot::ORM].get(),
		deferredTextures[DeferredSlot::Normal].get(),
		deferredTextures[DeferredSlot::PositionW].get(),
		deferredTextures[DeferredSlot::Emissive].get(),
		deferredTextures[DeferredSlot::PreCalculatedLight].get(),
		deferredTextures[DeferredSlot::Mask].get()
	};

	renderer.SetViewport(_renderResources->width, _renderResources->height);
	renderer.SetRenderTargets(DeferredSlot::Count, textures, _renderResources->depthTexture.get(), false);
	renderer.Clear(deferredTextures[DeferredSlot::Albedo]->GetClearValue());

}

void core::PreRenderSystem::preStartSystem(const OnPreStartSystem& event)
{
	auto& config = event.scene->GetRegistry()->ctx().get<core::Configuration>();

	if (event.scene->GetRegistry()->ctx().contains<core::RenderResources>())
		_renderResources = &event.scene->GetRegistry()->ctx().get<core::RenderResources>();
	else
		_renderResources = &event.scene->GetRegistry()->ctx().emplace<core::RenderResources>();

	_renderResources->width = config.width;
	_renderResources->height = config.height;

	float rtClearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

	// 공유 자원 초기화
	event.renderer->ReleaseTexture("SceneRT");
	_renderResources->renderTarget = event.renderer->CreateEmptyTexture("SceneRT", Texture::Type::Texture2D,
	                                                                    _renderResources->width,
	                                                                    _renderResources->height, 1,
	                                                                    Texture::Format::R8G8B8A8_UNORM,
	                                                                    Texture::Usage::RTV | Texture::Usage::UAV,
	                                                                    rtClearColor);



	event.renderer->ReleaseTexture("MainDepthStencil");
	TextureDesc mainDSVDesc("MainDepthStencil", Texture::Type::Texture2D, _renderResources->width, _renderResources->height, 1, Texture::Format::R24G8_TYPELESS, Texture::Usage::DSV);
	mainDSVDesc.dsvFormat = Texture::Format::D24_UNORM_S8_UINT;
	mainDSVDesc.srvFormat = Texture::Format::R24_UNORM_X8_TYPELESS;
	_renderResources->depthTexture = event.renderer->CreateEmptyTexture(mainDSVDesc);

	_renderResources->quadMaterial = Material::Create(event.renderer->LoadShader("./Shaders/unlit.hlsl"));
	_renderResources->quadMaterial->SetTexture("gDiffuseMap", _renderResources->renderTarget);



	_renderResources->quadMesh = event.renderer->GetMesh("Quad");
	if (_renderResources->quadMesh == nullptr)
	{
		auto quadMesh = MeshGenerator::CreateQuad({ -1, -1 }, { 1, 1 }, 0);
		event.renderer->AddMesh(quadMesh);
		_renderResources->quadMesh = quadMesh;
	}

	_renderResources->cubeMesh = event.renderer->GetMesh("Cube");


}

void core::PreRenderSystem::startSystem(const OnStartSystem& event)
{

}

void core::PreRenderSystem::finishSystem(const OnFinishSystem& event)
{
	_renderResources = nullptr;
}

void core::PreRenderSystem::destroyRenderResources(const OnDestroyRenderResources& event)
{
	_renderResources = nullptr;
}

void core::PreRenderSystem::createRenderResources(const OnCreateRenderResources& event)
{
	preStartSystem(OnPreStartSystem{ (*event.scene), event.renderer });
}
