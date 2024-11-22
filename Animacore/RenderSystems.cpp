#include "pch.h"
#include "RenderSystems.h"

#include "Scene.h"
#include "RenderComponents.h"
#include "CoreComponents.h"
#include "LightStructure.h"

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

#pragma region RenderSystem
core::RenderSystem::RenderSystem(Scene& scene) :
	ISystem(scene)
{
	_dispatcher = scene.GetDispatcher();
	_dispatcher->sink<OnChangeMesh>().connect<&RenderSystem::changeMesh>(this);
	_dispatcher->sink<OnPreStartSystem>().connect<&RenderSystem::preStartSystem>(this);
	_dispatcher->sink<OnFinishSystem>().connect<&RenderSystem::finishSystem>(this);
	_dispatcher->sink<OnCreateEntity>().connect<&RenderSystem::createEntity>(this);

	_dispatcher->sink<OnCreateRenderResources>().connect<&RenderSystem::createRenderResources>(this);
	_dispatcher->sink<OnDestroyRenderResources>().connect<&RenderSystem::destroyRenderResources>(this);
}

void core::RenderSystem::changeMesh(const OnChangeMesh& event)
{
	auto& meshRenderer = event.entity.Get<core::MeshRenderer>();
	meshRenderer.mesh = event.renderer->GetMesh(meshRenderer.meshString);
}

void core::RenderSystem::destroyRenderResources(const OnDestroyRenderResources& event)
{
	_renderResources = nullptr;
}

void core::RenderSystem::createRenderResources(const OnCreateRenderResources& event)
{
	preStartSystem(OnPreStartSystem{ (*event.scene), event.renderer });

}

void core::RenderSystem::preStartSystem(const OnPreStartSystem& event)
{
	auto& config = event.scene->GetRegistry()->ctx().get<core::Configuration>();
	auto&& registry = event.scene->GetRegistry();


	const std::array<std::string, DeferredSlot::Count> textureNames = {
	"gAlbedoTexture",
	"gORMTexture",
	"gNormalTexture",
	"gPositionWTexture",
	"gEmissiveTexture",
	"gPreCalculatedLightTexture",
	"gRecieveDecalTexture"
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

	float rtClearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

	core::RenderResources* renderResources = nullptr;
	if (event.scene->GetRegistry()->ctx().contains<core::RenderResources>())
		renderResources = &event.scene->GetRegistry()->ctx().get<core::RenderResources>();
	else
		renderResources = &event.scene->GetRegistry()->ctx().emplace<core::RenderResources>();

	_renderResources = renderResources;

	auto width = config.width;
	auto height = config.height;
	renderResources->width = width;
	renderResources->height = height;

	auto& deferredTextures = renderResources->deferredTextures;

	if (deferredTextures.size() == 0)
	{
		deferredTextures.reserve(DeferredSlot::Count);

		for (int i = 0; i < DeferredSlot::Count; i++)
			deferredTextures.push_back(event.renderer->CreateEmptyTexture(textureNames[i], Texture::Type::Texture2D, width, height, 1, textureFormats[i], Texture::Usage::RTV, rtClearColor));
	}
	else
	{
		if (deferredTextures[0]->GetWidth() != width || deferredTextures[0]->GetHeight() != height)
		{
			for (int i = 0; i < DeferredSlot::Count; i++)
				event.renderer->ReleaseTexture(textureNames[i]);

			deferredTextures.clear();
			deferredTextures.reserve(DeferredSlot::Count);

			for (int i = 0; i < DeferredSlot::Count; i++)
				deferredTextures.push_back(event.renderer->CreateEmptyTexture(textureNames[i], Texture::Type::Texture2D, width, height, 1, textureFormats[i], Texture::Usage::RTV, rtClearColor));
		}
	}

	// 초기 할당
	for (auto&& [entity, meshRenderer] : registry->view<core::MeshRenderer>().each())
	{
		if (meshRenderer.mesh == nullptr)
		{
			meshRenderer.mesh = event.renderer->GetMesh(meshRenderer.meshString);
		}

		if (meshRenderer.materials.size() <= 0)
		{
			for (auto&& materialString : meshRenderer.materialStrings)
			{
				meshRenderer.materials.push_back(event.renderer->GetMaterial(materialString));
			}
		}
	}
}

void core::RenderSystem::startSystem(const OnStartSystem& event)
{

}

void core::RenderSystem::finishSystem(const OnFinishSystem& event)
{
	_renderResources = nullptr;
}

void core::RenderSystem::createEntity(const OnCreateEntity& event)
{

}

void core::RenderSystem::operator()(Scene& scene, Renderer& renderer, float tick)
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

	dissolveFactor.gDissolveColor = Vector3(1.0f, 0.0f, 0.0f);
	dissolveFactor.gDissolveFactor = 0.0f;
	dissolveFactor.gEdgeWidth = -0.1f;

	{
		auto&& attributeView = registry.view<core::RenderAttributes, core::MeshRenderer>();
		for (auto&& [et, ra, mr] : attributeView.each())
		{
			if (ra.flags & core::RenderAttributes::Flag::IsTransparent || ra.flags & core::RenderAttributes::Flag::IsWater)
			{
				mr.isForward = true;
			}
		}
	}

	// deferredGeometry Pass
	{
		auto&& geometryViewIter = registry.view<core::WorldTransform, core::MeshRenderer>().each();
		for (auto&& [entity, transform, meshRenderer] : geometryViewIter)
		{
			if (!meshRenderer.isOn || meshRenderer.isForward || meshRenderer.isCustom)
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

				material->m_Shader->MapConstantBuffer(renderer.GetContext());
				material->m_Shader->SetConstant(CB_PER_OBJECT, &perObject, sizeof(PerObject));
				material->m_Shader->SetFloat3(G_EYE_POS_W, cameraTransform.position);
				material->m_Shader->SetInt(G_RECIEVE_DECAL, meshRenderer.canReceivingDecal);
				material->m_Shader->SetFloat(G_EMISSIVE_FACTOR, meshRenderer.emissiveFactor);
				material->m_Shader->SetConstant(CB_DISSOLVE_FACTOR, &dissolveFactor, sizeof(cbDissolveFactor));

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
}
