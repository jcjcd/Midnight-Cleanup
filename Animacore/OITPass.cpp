#include "pch.h"
#include "OITPass.h"

#include "Scene.h"
#include "RenderComponents.h"
#include "CoreComponents.h"

#include "../Animavision/Renderer.h"
#include "../Animavision/ShaderResource.h"
#include "../Animavision/Material.h"
#include "../Animavision/Shader.h"
#include "../Animavision/Mesh.h"

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

void core::OITPass::initOITResources(Scene& scene, Renderer& renderer, uint32_t width, uint32_t height)
{
	float accClearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float rvlClearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	_revealageTexture = renderer.CreateEmptyTexture("RevealageTexture", Texture::Type::Texture2D, width, height, 1, Texture::Format::R8G8B8A8_UNORM, Texture::Usage::RTV | Texture::Usage::UAV, rvlClearColor);
	_accumulationTexture = renderer.CreateEmptyTexture("AccumulationTexture", Texture::Type::Texture2D, width, height, 1, Texture::Format::R32G32B32A32_FLOAT, Texture::Usage::RTV | Texture::Usage::UAV, accClearColor);

	_oitCombineMaterial = Material::Create(renderer.LoadShader("./Shaders/TransparencyComposite.hlsl"));
	_oitCombineMaterial->SetTexture("gReveal", _revealageTexture);
	_oitCombineMaterial->SetTexture("gAccum", _accumulationTexture);
}

void core::OITPass::oitPass(core::Scene& scene, Renderer& renderer, const Matrix& view, const Matrix& proj, const Matrix& viewProj, const Vector3& camPos)
{
	auto& registry = *scene.GetRegistry();
	PerObject perObject;
	RenderResources& renderResources = registry.ctx().get<core::RenderResources>();

	{
		Texture* textures[] = { _revealageTexture.get(), _accumulationTexture.get() };
		renderer.SetRenderTargets(2, textures, renderResources.depthTexture.get(), false);
		renderer.ClearTexture(_revealageTexture.get(), _revealageTexture->GetClearValue());
		renderer.ClearTexture(_accumulationTexture.get(), _accumulationTexture->GetClearValue());
		renderer.ApplyRenderState(BlendState::TRANSPARENT_BLEND, RasterizerState::CULL_NONE, DepthStencilState::DEPTH_ENABLED);

		for (auto&& [entity, transform, meshRenderer, renderAttributes] : registry.view<core::WorldTransform, core::MeshRenderer, core::RenderAttributes>().each())
		{
			if (!meshRenderer.isOn)
				continue;

			if (!(renderAttributes.flags & core::RenderAttributes::Flag::IsTransparent) 
				|| renderAttributes.flags & core::RenderAttributes::Flag::IsWater)
				continue;

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

			auto& world = transform.matrix;
			auto worldInvTranspose = world.Invert().Transpose();

			for (uint32_t i = 0; i < (uint32_t)meshRenderer.materials.size(); ++i)
			{
				auto material = meshRenderer.materials[i];
				if (material == nullptr)
				{
					continue;
				}
				if (i >= meshRenderer.mesh->subMeshCount)
				{
					OutputDebugStringA(std::to_string(uint32_t(entity)).c_str());
					OutputDebugStringA("¡¸Submesh index out of range¡¹\n");
					continue;
				}

				perObject.gProj = proj;
				perObject.gView = view;
				perObject.gViewProj = viewProj;
				perObject.gWorld = world;
				perObject.gWorldInvTranspose = worldInvTranspose;
				perObject.gTexTransform = Matrix::Identity;

				material->m_Shader->MapConstantBuffer(renderer.GetContext());
				material->m_Shader->SetConstant("cbPerObject", &perObject, sizeof(PerObject));
				material->m_Shader->SetFloat3("gEyePosW", camPos);
				material->m_Shader->SetInt("gRecieveDecal", meshRenderer.canReceivingDecal);
				material->m_Shader->SetInt("gReflect", false);
				material->m_Shader->SetInt("gRefract", false);

				material->m_Shader->UnmapConstantBuffer(renderer.GetContext());

				renderer.Submit(*meshRenderer.mesh, *material, i, PrimitiveTopology::TRIANGLELIST, 1);
			}
		}
	}
}

void core::OITPass::oitCompositePass(core::Scene& scene, Renderer& renderer, const Matrix& view, const Matrix& proj, const Matrix& viewProj, std::shared_ptr<Mesh> quadMesh, std::shared_ptr<Texture> renderTargetTexture)
{
	Texture* textures[] = { renderTargetTexture.get() };
	renderer.SetRenderTargets(1, textures, nullptr, false);
	renderer.ApplyRenderState(BlendState::ALPHA_BLEND, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_ENABLED);

	auto world = Matrix::Identity;
	auto worldInvTranspose = Matrix::Identity;

	_oitCombineMaterial->m_Shader->MapConstantBuffer(renderer.GetContext());
	_oitCombineMaterial->m_Shader->SetMatrix("gWorld", world);
	_oitCombineMaterial->m_Shader->SetMatrix("gWorldInvTranspose", worldInvTranspose);
	_oitCombineMaterial->m_Shader->SetMatrix("gView", view);
	_oitCombineMaterial->m_Shader->SetMatrix("gViewProj", viewProj);
	_oitCombineMaterial->m_Shader->SetMatrix("gTexTransform", Matrix::Identity);
	_oitCombineMaterial->m_Shader->UnmapConstantBuffer(renderer.GetContext());


	renderer.Submit(*quadMesh, *_oitCombineMaterial, 0, PrimitiveTopology::TRIANGLELIST, 1);
}

void core::OITPass::finishOITResources()
{
	_revealageTexture.reset();
	_accumulationTexture.reset();
	_oitCombineMaterial.reset();
}
