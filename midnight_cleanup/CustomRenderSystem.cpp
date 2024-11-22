#include "pch.h"
#include "CustomRenderSystem.h"

#include <Animacore/Scene.h>
#include <Animacore/CoreComponents.h>
#include <Animacore/RenderComponents.h>
#include "McComponents.h"

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

	struct cbDissolveFactor
	{
		Vector3 gDissolveColor;
		float gDissolveFactor;
		float gEdgeWidth;
	};

}


mc::CustomRenderSystem::CustomRenderSystem(core::Scene& scene)
	: ISystem(scene)
{
	_dispatcher = scene.GetDispatcher();
	_dispatcher->sink<core::OnPreStartSystem>().connect<&CustomRenderSystem::preStartSystem>(this);
	_dispatcher->sink<core::OnFinishSystem>().connect<&CustomRenderSystem::finishSystem>(this);

}

mc::CustomRenderSystem::~CustomRenderSystem()
{
	_dispatcher->disconnect(this);
}

void mc::CustomRenderSystem::operator()(core::Scene& scene, Renderer& renderer, float tick)
{
	auto& registry = *scene.GetRegistry();
	auto& renderRes = scene.GetRegistry()->ctx().get<core::RenderResources>();
	const Matrix& view = renderRes.viewMatrix;
	const Matrix& proj = renderRes.projectionMatrix;
	const Matrix& viewProj = renderRes.viewProjectionMatrix;
	auto& cameraTransform = *renderRes.cameraTransform;

	DirectX::BoundingFrustum frustum;
	DirectX::BoundingFrustum::CreateFromMatrix(frustum, proj);
	frustum.Transform(frustum, view.Invert());


	PerObject perObject;
	cbDissolveFactor dissolveFactor;

	renderer.ApplyRenderState(BlendState::COUNT, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_ENABLED);

	{
		auto&& dissolveViewIter = registry.view<mc::DissolveRenderer, core::WorldTransform, core::MeshRenderer>().each();
		for (auto&& [entity, dissolveRenderer, transform, meshRenderer] : dissolveViewIter)
		{
			if (!meshRenderer.isOn || meshRenderer.isForward)
				continue;

			if (meshRenderer.mesh == nullptr)
			{
				meshRenderer.mesh = renderer.GetMesh(meshRenderer.meshString);
				continue;
			}

			DirectX::BoundingSphere boundingSphere = meshRenderer.mesh->boundingSphere;
			boundingSphere.Transform(boundingSphere, transform.matrix);

			if (frustum.Intersects(boundingSphere) == DirectX::DISJOINT && !meshRenderer.isSkinned)
				continue;

			if (!meshRenderer.isCulling)
				renderer.ApplyRenderState(BlendState::COUNT, RasterizerState::CULL_NONE, DepthStencilState::DEPTH_ENABLED);
			else
				renderer.ApplyRenderState(BlendState::COUNT, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_ENABLED);

			if (meshRenderer.materials.size() <= 0)
			{
				for (auto&& materialString : meshRenderer.materialStrings)
				{
					meshRenderer.materials.push_back(renderer.GetMaterial(materialString));
				}
				continue;
			}

			auto world = transform.matrix;

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
					OutputDebugStringA(std::to_string(static_cast<uint32_t>(entity)).c_str());
					OutputDebugStringA("「Submesh index out of range」\n");
					continue;
				}

				perObject.gWorld = world;
				perObject.gWorldInvTranspose = worldInvTranspose;
				perObject.gTexTransform = Matrix::Identity;
				perObject.gView = view;
				perObject.gProj = proj;
				perObject.gViewProj = viewProj;

				dissolveFactor.gDissolveColor = dissolveRenderer.dissolveColor;
				dissolveFactor.gDissolveFactor = dissolveRenderer.dissolveFactor;
				dissolveFactor.gEdgeWidth = dissolveRenderer.edgeWidth;

				material->m_Shader->MapConstantBuffer(renderer.GetContext());
				material->m_Shader->SetConstant(CB_PER_OBJECT, &perObject, sizeof(PerObject));
				material->m_Shader->SetFloat3(G_EYE_POS_W, cameraTransform.position);
				material->m_Shader->SetInt(G_RECIEVE_DECAL, meshRenderer.canReceivingDecal);
				material->m_Shader->SetFloat(G_EMISSIVE_FACTOR, meshRenderer.emissiveFactor);
				material->m_Shader->SetConstant("cbDissolveFactor", &dissolveFactor, sizeof(cbDissolveFactor));

				if (meshRenderer.isSkinned)
				{
					SubMeshDescriptor& subMesh = meshRenderer.mesh->subMeshDescriptors[i];
					if (meshRenderer.animator)
					{
						std::vector<Matrix> boneTransforms;
						boneTransforms.reserve(subMesh.boneIndexMap.size());

						for (int j = 0; j < meshRenderer.bones[i].size(); j++)
						{
							auto boneMatrix = meshRenderer.boneOffsets[i][j] * (meshRenderer.bones[i][j] ? meshRenderer.bones[i][j]->matrix : Matrix::Identity);
							boneTransforms.push_back(boneMatrix);
						}
						material->m_Shader->SetConstant("BoneMatrixBuffer", boneTransforms.data(), static_cast<uint32_t>(sizeof(Matrix) * boneTransforms.size()));
						material->m_Shader->SetMatrix("gWorld", Matrix::Identity);
					}
					else
					{
						std::vector<Matrix> boneTransforms;
						boneTransforms.resize(subMesh.boneIndexMap.size(), Matrix::Identity);
						material->m_Shader->SetConstant("BoneMatrixBuffer", boneTransforms.data(), static_cast<uint32_t>(sizeof(Matrix) * boneTransforms.size()));
					}
				}

				material->m_Shader->UnmapConstantBuffer(renderer.GetContext());

				renderer.Submit(*meshRenderer.mesh, *material, i, PrimitiveTopology::TRIANGLELIST, 1);
			}
		}
	}

	{
		for (auto&& [entity, videoRenderer, meshRenderer] : registry.view<mc::VideoRenderer, core::MeshRenderer>().each())
		{

			if (!meshRenderer.isOn || meshRenderer.isForward)
				continue;

			if (meshRenderer.mesh == nullptr)
			{
				meshRenderer.mesh = renderer.GetMesh(meshRenderer.meshString);
				continue;
			}

			DirectX::BoundingSphere boundingSphere = meshRenderer.mesh->boundingSphere;
			boundingSphere.Transform(boundingSphere, registry.get<core::WorldTransform>(entity).matrix);

			if (frustum.Intersects(boundingSphere) == DirectX::DISJOINT && !meshRenderer.isSkinned)
				continue;

			if (!meshRenderer.isCulling)
				renderer.ApplyRenderState(BlendState::COUNT, RasterizerState::CULL_NONE, DepthStencilState::DEPTH_ENABLED);
			else
				renderer.ApplyRenderState(BlendState::COUNT, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_ENABLED);

			if (meshRenderer.materials.size() <= 0)
			{
				for (auto&& materialString : meshRenderer.materialStrings)
				{
					meshRenderer.materials.push_back(renderer.GetMaterial(materialString));
				}
				continue;
			}

			perObject.gWorld = Matrix::Identity;
			perObject.gWorldInvTranspose = Matrix::Identity;
			perObject.gTexTransform = Matrix::Identity;
			perObject.gView = Matrix::Identity;
			perObject.gProj = Matrix::Identity;
			perObject.gViewProj = Matrix::Identity;

			for (uint32_t i = 0; i < static_cast<uint32_t>(meshRenderer.materials.size()); ++i)
			{
				auto material = meshRenderer.materials[i];
				if (material == nullptr)
				{
					continue;
				}
				if (i >= meshRenderer.mesh->subMeshCount)
				{
					OutputDebugStringA(std::to_string(static_cast<uint32_t>(entity)).c_str());
					OutputDebugStringA("「Submesh index out of range」\n");
					continue;
				}
				material->SetTexture("txYUV", _videoTextures[entity].getTexture());
				material->m_Shader->MapConstantBuffer(renderer.GetContext());
				material->m_Shader->SetConstant(CB_PER_OBJECT, &perObject, sizeof(PerObject));
				material->m_Shader->UnmapConstantBuffer(renderer.GetContext());

				renderer.Submit(*meshRenderer.mesh, *material, i, PrimitiveTopology::TRIANGLELIST, 1);
			}

		}
	}
}

void mc::CustomRenderSystem::operator()(core::Scene& scene, float tick)
{
	auto& registry = *scene.GetRegistry();

	auto&& input = scene.GetRegistry()->ctx().get<core::Input>();

	if (!_skip1Frame)
	{
		_skip1Frame = true;
		for (auto&& [entity, videoRenderer, meshRenderer, sound] : registry.view<mc::VideoRenderer, core::MeshRenderer, core::Sound>().each())
		{
			_videoTextures[entity].resume();
			sound.isPlaying = true;
			// 패치
			registry.patch<core::Sound>(entity);
		}
		return;
	}

	for (auto&& [entity, videoRenderer, meshRenderer] : registry.view<mc::VideoRenderer, core::MeshRenderer>().each())
	{
		if (input.keyStates[VK_ESCAPE] == core::Input::State::Down)
		{
			_dispatcher->enqueue<core::OnChangeScene>("./Resources/Scenes/InGame.scene");
		}

		if (_videoTextures[entity].update(tick))
		{
			_dispatcher->enqueue<core::OnChangeScene>("./Resources/Scenes/InGame.scene");
		}
	}
}

void mc::CustomRenderSystem::operator()(core::Scene& scene)
{



}


void mc::CustomRenderSystem::preStartSystem(const core::OnPreStartSystem& event)
{
	for (auto&& [entity, dissolveRenderer, meshRenderer] : event.scene->GetRegistry()->view<mc::DissolveRenderer, core::MeshRenderer>().each())
	{
		meshRenderer.isCustom = true;
	}

	for (auto&& [entity, videoRenderer, meshRenderer] : event.scene->GetRegistry()->view<mc::VideoRenderer, core::MeshRenderer>().each())
	{
		meshRenderer.isCustom = true;
		_videoTextures.emplace(entity, VideoTexture{});
		_videoTextures[entity].create(videoRenderer.videoPath.c_str(), event.renderer);
		_videoTextures[entity].pause();
	}
}

void mc::CustomRenderSystem::finishSystem(const core::OnFinishSystem& event)
{
	_skip1Frame = false;
	_videoTextures.clear();

	// 모든 사운드의 is playing을 끈다.

	for (auto&& [entity, sound] : event.scene->GetRegistry()->view<core::Sound>().each())
	{
		sound.isPlaying = false;
		// 패치
		event.scene->GetRegistry()->patch<core::Sound>(entity);
	}
}
