#include "pch.h"
#include "RaytraceRenderSystem.h"

#include "Scene.h"
#include "RenderComponents.h"
#include "CoreComponents.h"
#include "LightStructure.h"

#include "../Animavision/Renderer.h"
#include "../Animavision/ShaderResource.h"
#include "../Animavision/Material.h"
#include "../Animavision/Shader.h"
#include "../Animavision/Mesh.h"


core::RaytraceRenderSystem::RaytraceRenderSystem(Scene& scene) :
	ISystem(scene)
{
	_dispatcher = scene.GetDispatcher();
	_dispatcher->sink<OnChangeMesh>().connect<&RaytraceRenderSystem::changeMesh>(this);
	_dispatcher->sink<OnStartSystem>().connect<&RaytraceRenderSystem::startSystem>(this);
	_dispatcher->sink<OnFinishSystem>().connect<&RaytraceRenderSystem::finishSystem>(this);
	_dispatcher->sink<OnCreateEntity>().connect<&RaytraceRenderSystem::createEntity>(this);
}

void core::RaytraceRenderSystem::operator()(Scene& scene, Renderer& renderer, float tick)
{
	// 라이트와 친구들도 추가해주어야한다...

	//_renderer->FlushRaytracingData();

	auto& registry = *scene.GetRegistry();



	renderer.ResetAS();

	for (auto&& [entity, transform, meshRenderer] : registry.view<core::WorldTransform, core::MeshRenderer>().each())
	{
		if (meshRenderer.mesh == nullptr)
		{
			meshRenderer.mesh = renderer.GetMesh(meshRenderer.meshString);
			continue;
		}

		if (meshRenderer.materials.size() <= 0)
		{
			for (auto&& materialString : meshRenderer.materialStrings)
			{
				meshRenderer.materials.push_back(renderer.GetMaterial(materialString));
			}
			continue;
		}

		// 창 : 이거 레이트레이싱 공부하면 수정해야됨
		if (meshRenderer.isSkinned && meshRenderer.animator)
		{
			std::vector<Matrix> boneTransforms;
			//boneTransforms.reserve(meshRenderer.mesh->boneIndexMap.size());
			if (meshRenderer.animator->_currentNodeClips)
			{
				for (int i = 0; i < meshRenderer.animator->_currentNodeClips->size(); i++)
				{
					auto [transform, clip] = meshRenderer.animator->_currentNodeClips->at(i);
					auto boneMatrix = meshRenderer.animator->_boneOffsets[i] * transform->matrix;
					boneTransforms.push_back(boneMatrix);
				}
				renderer.ComputeSkinnedVertices(*meshRenderer.mesh, boneTransforms, meshRenderer.mesh->GetVertexCount());
				renderer.ModifyBLASGeometry((uint32_t)entity, *meshRenderer.mesh);
			}
			renderer.AddSkinnedBLASInstance((uint32_t)entity, meshRenderer.hitDistribution, &transform.matrix.m[0][0]);
		}
		else
		{
			renderer.AddBottomLevelASInstance(meshRenderer.meshString, meshRenderer.hitDistribution, &transform.matrix.m[0][0]);
		}
	}

	renderer.UpdateAccelerationStructures();

	Camera* mainCam = nullptr;
	WorldTransform* cameraTransform = nullptr;

	for (auto&& [entity, transform, camera] : registry.view<core::WorldTransform, core::Camera>().each())
	{
		if (camera.isActive)
		{
			mainCam = &camera;
			cameraTransform = &transform;
			break;
		}
	}

	if (mainCam != nullptr)
	{
		Matrix proj = DirectX::XMMatrixPerspectiveFovLH(
			mainCam->fieldOfView,
			mainCam->aspectRatio,
			mainCam->nearClip,
			mainCam->farClip);

		renderer.SetCamera(
			cameraTransform->matrix.Invert(),
			proj,
			cameraTransform->position,
			-cameraTransform->matrix.Forward(),
			cameraTransform->matrix.Up(),
			mainCam->nearClip,
			mainCam->farClip);

	}


	renderer.DoRayTracing();

	{
		renderer.SetViewport(_width, _height);

		float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderer.SetRenderTargets(1, nullptr);
		renderer.Clear(clearColor);

		renderer.ApplyRenderState(BlendState::NO_BLEND, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_ENABLED);

		auto&& quadshader = _quadMaterial->GetShader();
		quadshader->SetMatrix("gWorld", Matrix::Identity);
		quadshader->SetMatrix("gView", Matrix::Identity);
		quadshader->SetMatrix("gViewProj", Matrix::Identity);
		quadshader->SetMatrix("gTexTransform", Matrix::Identity);

		renderer.Submit(*_quadMesh, *_quadMaterial, PrimitiveTopology::TRIANGLELIST, 1);
	}
}

void core::RaytraceRenderSystem::startSystem(const OnStartSystem& event)
{
	// 	if (_isRendererModified)
	// 	{
	auto* renderer = event.renderer;
	auto& registry = *event.scene->GetRegistry();

	_quadMaterial = Material::Create(event.renderer->LoadShader("./Shaders/unlit.hlsl"));
	_quadMaterial->SetTexture("gDiffuseMap", renderer->GetColorRt());

	_quadMesh = event.renderer->GetMesh("Quad");
	if (_quadMesh == nullptr)
	{
		auto quadMesh = MeshGenerator::CreateQuad({ -1, -1 }, { 1, 1 }, 0);
		event.renderer->AddMesh(quadMesh);
		_quadMesh = quadMesh;
	}


	renderer->FlushRaytracingData();

	//처음부터 다만든다
	for (auto&& [entity, meshRenderer] : registry.view<core::MeshRenderer>().each())
	{
		if (meshRenderer.mesh == nullptr)
		{
			meshRenderer.mesh = renderer->GetMesh(meshRenderer.meshString);
			continue;
		}

		if (meshRenderer.materials.size() <= 0)
		{
			for (auto&& materialString : meshRenderer.materialStrings)
			{
				meshRenderer.materials.push_back(renderer->GetMaterial(materialString));
			}
			continue;
		}


		// 이부분은 사실 따로 빼는게 나을수도?
		if (meshRenderer.isSkinned)
		{
			renderer->AddSkinnedBottomLevelAS((uint32_t)entity, *meshRenderer.mesh, true);
		}

		renderer->AddMeshToScene(meshRenderer.mesh, meshRenderer.materials, &meshRenderer.hitDistribution, false);
	}

	// SBT빌드
	renderer->AddMeshToScene(nullptr, {}, nullptr, true);
	// 	}
}

void core::RaytraceRenderSystem::finishSystem(const OnFinishSystem& event)
{
	event.renderer->ResetSkinnedBLAS();
}

void core::RaytraceRenderSystem::createEntity(const OnCreateEntity& event)
{

}

void core::RaytraceRenderSystem::changeMesh(const OnChangeMesh& event)
{

}