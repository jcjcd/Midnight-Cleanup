#include "pch.h"
#include "NeoWooDXI.h"
#include "DX11State.h"
#include "Mesh.h"
#include "DX11Sampler.h"
#include "DX11Buffer.h"
#include "ResourceFontLoader.h"

#include <functional>
#include <DirectXTex.h>
#include <wincodec.h>
#include <fstream>

#include <./directxtk/Effects.h>
#include <./directxtk/DirectXHelpers.h>

#include "VideoTexture.h"

using ParticleBuffers = std::tuple<std::shared_ptr<Texture>, std::shared_ptr<Texture>, std::shared_ptr<Texture>>;

void NeoWooDXI::Resize(uint32_t width, uint32_t height)
{
	m_Context->Resize(width, height);
}

void NeoWooDXI::SetFullScreen(bool fullscreen)
{
	m_Context->SetFullScreen(fullscreen);
}

void NeoWooDXI::SetVSync(bool vsync)
{
	m_Context->SetVSync(vsync);
}

void NeoWooDXI::Submit(Mesh& mesh, Material& material, PrimitiveTopology primitiveMode, uint32_t instances)
{
	auto findIt = m_PipelineStates.find(material.m_Shader->ID);

	if (findIt == m_PipelineStates.end())
		__debugbreak();
	else
	{
		auto& pipelineState = findIt->second;
		pipelineState.Initialize(m_ResourceManager->CreateRasterizerState(m_RasterizerState), m_ResourceManager->CreateDepthStencilState(m_DepthStencilState), m_ResourceManager->CreateBlendState(m_BlendState));
		pipelineState.Bind(*m_Context.get());
	}

	Shader* shader = material.m_Shader.get();
	auto dx11Shader = static_cast<DX11Shader*>(shader);

	shader->Bind(this);

	bindShaderResources(material);

	m_Context->SetPrimitiveTopology(static_cast<D3D11_PRIMITIVE_TOPOLOGY>(primitiveMode));

	mesh.vertexBuffer->Bind(m_Context.get());
	mesh.indexBuffer->Bind(m_Context.get());

	for (uint32_t i = 0; i < mesh.subMeshDescriptors.size(); ++i)
	{
		auto& subMesh = mesh.subMeshDescriptors[i];
		m_Context->DrawIndexedInstanced(subMesh.indexCount, instances, subMesh.indexOffset, subMesh.vertexOffset, 0);
	}

	dx11Shader->Unbind(this);
	unbindShaderResources(material);
}

void NeoWooDXI::Submit(Mesh& mesh, Material& material, uint32_t subMeshIndex, PrimitiveTopology primitiveMode /*= PrimitiveTopology::TRIANGLELIST*/, uint32_t instances /*= 1*/)
{
	if (subMeshIndex >= mesh.subMeshDescriptors.size())
	{
		OutputDebugStringA(mesh.name.c_str());
		OutputDebugStringA(" : Invalid submesh index\n");
		return;
	}

	auto findIt = m_PipelineStates.find(material.m_Shader->ID);

	if (findIt == m_PipelineStates.end())
		__debugbreak();
	else
	{
		auto& pipelineState = findIt->second;
		pipelineState.Initialize(m_ResourceManager->CreateRasterizerState(m_RasterizerState),
		                         m_ResourceManager->CreateDepthStencilState(m_DepthStencilState),
		                         m_ResourceManager->CreateBlendState(m_BlendState));

		pipelineState.Bind(*m_Context.get());
	}

	Shader* shader = material.m_Shader.get();
	auto dx11Shader = static_cast<DX11Shader*>(shader);

	shader->Bind(this);

	bindShaderResources(material);

	const auto& subMesh = mesh.subMeshDescriptors[subMeshIndex];

	m_Context->SetPrimitiveTopology(static_cast<D3D11_PRIMITIVE_TOPOLOGY>(primitiveMode));
	mesh.vertexBuffer->Bind(m_Context.get());
	mesh.indexBuffer->Bind(m_Context.get());
	m_Context->DrawIndexedInstanced(subMesh.indexCount, instances, subMesh.indexOffset, subMesh.vertexOffset, 0);

#ifdef _DEBUG
	ID3D11Debug* debugDevice = nullptr;
	if (SUCCEEDED(m_Context->GetDevice()->QueryInterface(__uuidof(ID3D11Debug), (void**)&debugDevice))) {
		ID3D11InfoQueue* infoQueue = nullptr;
		if (SUCCEEDED(debugDevice->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&infoQueue))) {
			infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
			infoQueue->Release();
		}
		debugDevice->Release();
	}
#endif

	dx11Shader->Unbind(this);
	unbindShaderResources(material);
}

void NeoWooDXI::DispatchCompute(Material& material, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
	m_Context->GetDeviceContext()->OMSetRenderTargets(0, nullptr, nullptr);

	auto shader = material.m_Shader.get();
	DX11Shader* dx11Shader = static_cast<DX11Shader*>(shader);

	if (dx11Shader->GetComputeShader() == nullptr)
		__debugbreak();

	shader->Bind(this);

	bindShaderResources(material);
	m_Context->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);

	shader->Unbind(this);
	unbindShaderResources(material);
}

void NeoWooDXI::CopyStructureCount(Buffer* dest, uint32_t distAlignedByteOffset, Texture* src)
{
	DX11ConstantBuffer* dx11Buffer = static_cast<DX11ConstantBuffer*>(dest);
	DX11Texture* dx11Texture = static_cast<DX11Texture*>(src);
	assert(dx11Buffer && dx11Texture->GetUAV());

	m_Context->GetDeviceContext()->CopyStructureCount(dx11Buffer->GetBuffer(), distAlignedByteOffset, dx11Texture->GetUAV());
}

void NeoWooDXI::SubmitInstancedIndirect(Material& material, Texture* argsBuffer, uint32_t allignedByteOffsetForArgs, PrimitiveTopology primitiveMode/* = PrimitiveTopology::TRIANGLELIST*/)
{
	if (m_PipelineStates.contains(material.m_Shader->ID))
	{
		auto& pipelineState = m_PipelineStates[material.m_Shader->ID];
		pipelineState.Initialize(m_ResourceManager->CreateRasterizerState(m_RasterizerState), m_ResourceManager->CreateDepthStencilState(m_DepthStencilState), m_ResourceManager->CreateBlendState(m_BlendState));
		pipelineState.Bind(*m_Context.get());
	}
	else
		__debugbreak();

	Shader* shader = material.m_Shader.get();
	auto dx11Shader = static_cast<DX11Shader*>(shader);

	shader->Bind(this);

	bindShaderResources(material);

	ID3D11Buffer* vb = nullptr;
	uint32_t stride = 0;
	uint32_t offset = 0;
	m_Context->GetDeviceContext()->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	m_Context->GetDeviceContext()->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
	m_Context->GetDeviceContext()->IASetPrimitiveTopology(static_cast<D3D11_PRIMITIVE_TOPOLOGY>(primitiveMode));

	DX11Texture* dx11Buffer = static_cast<DX11Texture*>(argsBuffer);

	m_Context->DrawInstancedIndirect(dx11Buffer->GetBuffer(), allignedByteOffsetForArgs);

	dx11Shader->Unbind(this);
	unbindShaderResources(material);
}

void NeoWooDXI::BeginRender()
{
	m_Context->BeginRender();
}

void NeoWooDXI::EndRender()
{
	m_Context->EndRender();
}

void NeoWooDXI::Clear(const float* RGBA)
{
	m_Context->Clear(RGBA);
}

void NeoWooDXI::ClearTexture(Texture* texture, const float* clearColor)
{
	DX11Texture* dx11Texture = static_cast<DX11Texture*>(texture);

	if (HasFlag(dx11Texture->GetUsage(), Texture::Usage::RTV))
	{
		auto rtv = dx11Texture->GetRTV();
		m_Context->GetDeviceContext()->ClearRenderTargetView(rtv, clearColor);
	}
	if (HasFlag(dx11Texture->GetUsage(), Texture::Usage::DSV))
	{
		auto dsv = dx11Texture->GetDSV();
		m_Context->GetDeviceContext()->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}
}

std::shared_ptr<Texture> NeoWooDXI::CreateEmptyTexture(std::string name, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, Texture::Format format, Texture::Usage usage /*= Texture::Usage::NONE*/, float* clearColor /*= nullptr*/, Texture::UAVType uavType /*= Texture::UAVType::NONE*/, uint32_t arraySize /*= 1*/)
{
	return m_ResourceManager->CreateEmptyTexture(name, type, width, height, mipLevels, format, usage, clearColor, uavType, arraySize);
}

std::shared_ptr<Texture> NeoWooDXI::CreateEmptyTexture(const TextureDesc& desc)
{
	return m_ResourceManager->CreateEmptyTexture(desc);
}

void NeoWooDXI::SetRenderTargets(uint32_t numRenderTargets, Texture* renderTargets[], Texture* depthStencil /*= nullptr*/, bool useDefaultDSV /*= true*/)
{
	m_Context->SetRenderTargets(numRenderTargets, renderTargets, depthStencil, useDefaultDSV);
}

void NeoWooDXI::ApplyRenderState(BlendState blendstate, RasterizerState rasterizerstate, DepthStencilState depthstencilstate)
{
	m_BlendState = blendstate;
	m_RasterizerState = rasterizerstate;
	m_DepthStencilState = depthstencilstate;
}

void NeoWooDXI::SetViewport(uint32_t width, uint32_t height)
{
	m_Context->SetViewport(width, height);
}

NeoWooDXI::NeoWooDXI(HWND hwnd, uint32_t width, uint32_t height)
{
	m_Context = std::make_unique<NeoDX11Context>(hwnd, width, height);
	m_ResourceManager = std::make_unique<DX11ResourceManager>(m_Context.get());
	m_MaterialLibrary = std::make_unique<MaterialLibrary>();
	m_ShaderLibrary = std::make_unique<ShaderLibrary>();
	m_MeshLibrary = std::make_unique<MeshLibrary>(this);
	m_AnimationLibrary = std::make_unique<AnimationLibrary>();

	// DebugDraw
	m_states = std::make_unique<DirectX::DX11::CommonStates>(m_Context->GetDevice());

	m_effect = std::make_unique<DirectX::BasicEffect>(m_Context->GetDevice());
	m_effect->SetVertexColorEnabled(true);
	m_effect->SetVertexColorEnabled(true);

	HRESULT hr = DirectX::CreateInputLayoutFromEffect<VertexType>(m_Context->GetDevice(), m_effect.get(),
		m_inputLayout.ReleaseAndGetAddressOf());

	assert(SUCCEEDED(hr));

	auto context = m_Context->GetDeviceContext();
	m_batch = std::make_unique<DirectX::DX11::PrimitiveBatch<VertexType>>(context);

	VideoTexture::createAPI();
}

NeoWooDXI::~NeoWooDXI()
{
	/*for (const auto& [fontPath, font] : m_Fonts)
	{
		std::wstring wpath(fontPath.begin(), fontPath.end());
		RemoveFontResourceEx(wpath.c_str(), FR_NOT_ENUM, nullptr);
	}*/
	VideoTexture::destroyAPI();

	m_Fonts.clear();
	m_UITextures.clear();
	m_ParticleTextures.clear();
	m_PipelineStates.clear();
	m_ResourceManager.reset();
	m_AnimationLibrary.reset();
	m_MaterialLibrary.reset();
	m_ShaderLibrary.reset();
	m_MeshLibrary.reset();
	m_Context.reset();
}

RendererContext* NeoWooDXI::GetContext()
{
	return m_Context.get();
}

void* NeoWooDXI::GetShaderResourceView()
{
	return reinterpret_cast<void*>(m_Context->GetRenderTargetSRV());
}

void* NeoWooDXI::GetDevice()
{
	return reinterpret_cast<void*>(m_Context->GetDevice());
}

void* NeoWooDXI::GetDeviceContext()
{
	return reinterpret_cast<void*>(m_Context->GetDeviceContext());
}

std::shared_ptr<Texture> NeoWooDXI::GetTexture(const char* path)
{
	return m_ResourceManager->GetTexture(path);
}

std::shared_ptr<Texture> NeoWooDXI::CreateTexture(const char* path)
{
	return m_ResourceManager->CreateTexture2D(path);
}

std::shared_ptr<Texture> NeoWooDXI::CreateTexture2DArray(const char* path)
{
	return m_ResourceManager->CreateTexture2DArray(path);
}

std::shared_ptr<Texture> NeoWooDXI::CreateTextureCube(const char* path)
{
	return m_ResourceManager->CreateTextureCube(path);
}

void NeoWooDXI::ReleaseTexture(const std::string& name)
{
	m_ResourceManager->ReleaseTexture(name);
}

std::shared_ptr<Texture> NeoWooDXI::CreateLightmapTexture(const char* path)
{
	return m_ResourceManager->CreateLightmapTexture(path);
}

void NeoWooDXI::SaveLightmapTextureArray(std::vector<std::shared_ptr<Texture>> lightmaps, std::string path)
{
	std::shared_ptr<DX11Texture> texture = std::static_pointer_cast<DX11Texture>(lightmaps[0]);

	D3D11_TEXTURE2D_DESC texDesc = {};
	texture->GetTexture2D()->GetDesc(&texDesc);

	D3D11_TEXTURE2D_DESC arrayDesc = {};
	arrayDesc.Width = texDesc.Width;
	arrayDesc.Height = texDesc.Height;
	arrayDesc.MipLevels = texDesc.MipLevels;
	arrayDesc.ArraySize = static_cast<uint32_t>(lightmaps.size());
	arrayDesc.Format = texDesc.Format;
	arrayDesc.SampleDesc.Count = 1;
	arrayDesc.SampleDesc.Quality = 0;
	arrayDesc.Usage = D3D11_USAGE_DEFAULT;
	arrayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	std::shared_ptr<Texture> lightmap = std::make_shared<DX11Texture>();
	std::shared_ptr<DX11Texture> lightmapTexture = std::static_pointer_cast<DX11Texture>(lightmap);
	ThrowIfFailed(m_Context->GetDevice()->CreateTexture2D(&arrayDesc, nullptr, lightmapTexture->GetTexture2DAddress()));

	for (uint32_t i = 0; i < arrayDesc.ArraySize; i++)
	{
		for (uint32_t miplevel = 0; miplevel < texDesc.MipLevels; ++miplevel)
		{
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			std::shared_ptr<DX11Texture> lightmap = std::static_pointer_cast<DX11Texture>(lightmaps[i]);
			ThrowIfFailed(m_Context->GetDeviceContext()->Map(lightmap->GetTexture2D(), miplevel, D3D11_MAP_READ, 0, &mappedResource));

			m_Context->GetDeviceContext()->UpdateSubresource(lightmapTexture->GetTexture2D(), D3D11CalcSubresource(miplevel, i, texDesc.MipLevels), nullptr, mappedResource.pData, mappedResource.RowPitch, mappedResource.DepthPitch);

			m_Context->GetDeviceContext()->Unmap(lightmap->GetTexture2D(), miplevel);
		}
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = arrayDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Texture2DArray.MostDetailedMip = 0;
	srvDesc.Texture2DArray.MipLevels = arrayDesc.MipLevels;
	srvDesc.Texture2DArray.FirstArraySlice = 0;
	srvDesc.Texture2DArray.ArraySize = arrayDesc.ArraySize;

	ThrowIfFailed(m_Context->GetDevice()->CreateShaderResourceView(lightmapTexture->GetTexture2D(), &srvDesc, lightmapTexture->GetSRVAddress()));

	// Save lightmap texture to dds

	std::string lightmapPath = path + ".dds";
	DirectX::ScratchImage image;
	std::filesystem::path currentPath = std::filesystem::current_path();
	std::wstring wpath(lightmapPath.begin(), lightmapPath.end());
	wpath = currentPath.wstring() + wpath;
	ThrowIfFailed(DirectX::CaptureTexture(m_Context->GetDevice(), m_Context->GetDeviceContext(), lightmapTexture->GetTexture2D(), image));
	ThrowIfFailed(DirectX::SaveToDDSFile(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::DDS_FLAGS_NONE, wpath.c_str()));
}

std::shared_ptr<Shader> NeoWooDXI::LoadShader(const char* srcPath)
{
	auto shader = m_ShaderLibrary->LoadShader(this, srcPath);
	auto dx11Shader = std::static_pointer_cast<DX11Shader>(shader);
	if (shader)
	{
		m_PipelineStates.emplace(shader->ID, DX11PipelineState());
		return shader;
	}
	else
	{
		OutputDebugStringA("Failed to load shader\n");
		return nullptr;
	}
}

void NeoWooDXI::LoadMaterialsFromDrive(const std::string& path)
{
	std::filesystem::path directory = path;

	if (!std::filesystem::exists(directory))
	{
		return;
	}

	if (std::filesystem::is_directory(directory))
	{
		m_MaterialLibrary->LoadFromDirectory(path);
	}
	else
	{
		m_MaterialLibrary->LoadFromFile(path);
	}

	for (auto& material : m_MaterialLibrary->GetMaterials())
	{
		if (material.second->m_ShaderString.empty())
		{
			OutputDebugStringA("Material does not have a shader\n");
			continue;
		}
		material.second->m_Shader = LoadShader(material.second->m_ShaderString.c_str());

		if (material.second->m_Shader == nullptr)
		{
			OutputDebugStringA("Shader can't find.\n");
			assert(false);
			continue;
		}

		for (auto& texture : material.second->m_Textures)
		{
			// 창 : 이부분 큐브맵텍스쳐에 대해서도 지원해야한다..아마..
			if (texture.second.Path.empty())
			{
				OutputDebugStringA("Texture does not have a path\n");
				switch (texture.second.Type)
				{
				case Texture::Type::Texture2D:
					texture.second.Path = DEFAULT_TEXTURE2D_PATH;
					texture.second.Texture = m_ResourceManager->CreateTexture2D(DEFAULT_TEXTURE2D_PATH);
					break;
				case Texture::Type::TextureCube:
					texture.second.Path = DEFAULT_TEXTURECUBE_PATH;
					texture.second.Texture = m_ResourceManager->CreateTextureCube(DEFAULT_TEXTURECUBE_PATH);
					break;
				default:
					break;
				}
			}
			else
			{
				std::filesystem::path texturePath = texture.second.Path;
				if (texturePath.extension().string().empty())
				{

				}
				else
				{
					texture.second.Path = texturePath.string();
					texture.second.Texture = Texture::Create(this, texture.second.Path.c_str(), texture.second.Type);
				}
			}

			if (texture.second.Texture == nullptr)
			{
				OutputDebugStringA("Texture does not have a path\n");
				switch (texture.second.Type)
				{
				case Texture::Type::Texture2D:
					texture.second.Path = DEFAULT_TEXTURE2D_PATH;
					texture.second.Texture = m_ResourceManager->CreateTexture2D(DEFAULT_TEXTURE2D_PATH);
					break;
				case Texture::Type::TextureCube:
					texture.second.Path = DEFAULT_TEXTURECUBE_PATH;
					texture.second.Texture = m_ResourceManager->CreateTextureCube(DEFAULT_TEXTURECUBE_PATH);
					break;
				default:
					break;
				}
			}
		}

		// uav는 시리얼라이즈를 지금 안하니까 이렇게라도 가져오자
		auto&& dx11shader = std::static_pointer_cast<DX11Shader>(material.second->m_Shader);
		for (auto& uav : dx11shader->m_UAVBindings)
		{
			if (material.second->m_UAVTextures.find(uav.Name) == material.second->m_UAVTextures.end())
			{
				material.second->m_UAVTextures.emplace(uav.Name, nullptr);
			}
		}
	}
}

void NeoWooDXI::SaveMaterial(const std::string& name)
{
	auto material = m_MaterialLibrary->GetMaterial(name);

	for (auto& texture : material->m_Textures)
	{
		// 창 : 이부분 큐브맵텍스쳐에 대해서도 지원해야한다..아마..
		if (texture.second.Path.empty())
		{
			OutputDebugStringA("Texture does not have a path\n");
			switch (texture.second.Type)
			{
			case Texture::Type::Texture2D:
				texture.second.Path = DEFAULT_TEXTURE2D_PATH;
				texture.second.Texture = m_ResourceManager->CreateTexture2D(DEFAULT_TEXTURE2D_PATH);
				break;
			case Texture::Type::TextureCube:
				texture.second.Path = DEFAULT_TEXTURECUBE_PATH;
				texture.second.Texture = m_ResourceManager->CreateTextureCube(DEFAULT_TEXTURECUBE_PATH);
				break;
			case Texture::Type::Texture2DArray:
				texture.second.Path = DEFAULT_TEXTURE2DARRAY_PATH;
				texture.second.Texture = m_ResourceManager->CreateTexture2DArray(DEFAULT_TEXTURE2DARRAY_PATH);
				break;
			default:
				break;
			}
		}
		else
		{
			texture.second.Path = texture.second.Path.c_str();
			texture.second.Texture = Texture::Create(this, texture.second.Path.c_str(), texture.second.Type);
		}

		if (texture.second.Texture == nullptr)
		{
			OutputDebugStringA("Texture does not have a path\n");
			switch (texture.second.Type)
			{
			case Texture::Type::Texture2D:
				texture.second.Path = DEFAULT_TEXTURE2D_PATH;
				texture.second.Texture = m_ResourceManager->CreateTexture2D(DEFAULT_TEXTURE2D_PATH);
				break;
			case Texture::Type::TextureCube:
				texture.second.Path = DEFAULT_TEXTURECUBE_PATH;
				texture.second.Texture = m_ResourceManager->CreateTextureCube(DEFAULT_TEXTURECUBE_PATH);
				break;
			default:
				break;
			}
		}

		m_MaterialLibrary->SaveMaterial(name);
	}
}

void NeoWooDXI::LoadShadersFromDrive(const std::string& path)
{
	auto start = std::chrono::high_resolution_clock::now();

	std::filesystem::path shaderPath = path;
	if (!std::filesystem::exists(shaderPath))
	{
		OutputDebugStringA("Shader path does not exist\n");
		return;
	}

	for (const auto& entry : std::filesystem::recursive_directory_iterator(shaderPath))
	{
		if (entry.is_regular_file())
		{
			auto& path = entry.path();
			auto extension = path.extension();
			if (extension == ".hlsl")
			{
				auto pathStirng = path.string();
				std::replace(pathStirng.begin(), pathStirng.end(), '\\', '/');
				LoadShader(pathStirng.c_str());
			}
		}
	}

	auto end = std::chrono::high_resolution_clock::now();

	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

	OutputDebugStringA("LoadShadersFromDrive: ");
	OutputDebugStringA(std::to_string(duration).c_str());
	OutputDebugStringA("ms\n");
}

void NeoWooDXI::LoadMeshesFromDrive(const std::string& path)
{
	std::filesystem::path directory = path;

	if (!std::filesystem::exists(directory))
	{
		return;
	}

	if (std::filesystem::is_directory(directory))
	{
		m_MeshLibrary->LoadMeshesFromDirectory(path);
	}
	else
	{
		m_MeshLibrary->LoadMeshesFromFile(path);
	}
}

void NeoWooDXI::LoadAnimationClipsFromDrive(const std::string& path)
{
	std::filesystem::path directory = path;

	if (!std::filesystem::exists(directory))
		return;

	if (std::filesystem::is_directory(directory))
		m_AnimationLibrary->LoadAnimationClipsFromDirectory(path);
	else
		m_AnimationLibrary->LoadAnimationClipsFromFile(path);
}

void NeoWooDXI::LoadUITexturesFromDrive(const std::string& path)
{
	std::filesystem::path directory = path;

	if (!std::filesystem::exists(directory))
		return;

	if (std::filesystem::is_directory(directory))
		loadUITexturesFromDirectory(path);
	else
		loadUITexturesFromFile(path);
}

std::shared_ptr<Texture> NeoWooDXI::GetUITexture(const std::string& name)
{
	auto findIt = m_UITextures.find(name);

	if (findIt != m_UITextures.end())
		return findIt->second;

	return nullptr;
}

void NeoWooDXI::SetDebugViewProj(const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj)
{
	m_effect->SetView(view);
	m_effect->SetProjection(proj);
}

std::shared_ptr<Texture> NeoWooDXI::CreateRandomTexture()
{
	return m_ResourceManager->CreateRandomTexture();
}

std::shared_ptr<Texture> NeoWooDXI::CreateIndirectDrawTexture()
{
	return m_ResourceManager->CreateIndirectDrawTexture();
}

void NeoWooDXI::LoadParticleTexturesFromDrive(const std::string& path)
{
	std::filesystem::path directory = path;

	if (!std::filesystem::exists(directory))
		return;

	if (std::filesystem::is_directory(directory))
		loadParticleTexturesFromDirectory(path);
	else
		loadParticleTexturesFromFile(path);
}

std::shared_ptr<Texture> NeoWooDXI::GetParticleTexture(std::string name)
{
	if (m_ParticleTextures.contains(name))
		return m_ParticleTextures[name];

	return nullptr;
}

ParticleBuffers NeoWooDXI::CreateParticleBufferTextures(uint32_t maxCount, uint32_t entity)
{
	return m_ResourceManager->CreateParticleBuffers(maxCount, entity);
}

float NeoWooDXI::GetRandomSeed(float min, float max)
{
	return m_ResourceManager->GetRandomSeed(min, max);
}

void NeoWooDXI::SubmitText(Texture* rt, Font* font, const std::string& text, Vector2 posH, Vector2 size, Vector2 scale, DirectX::SimpleMath::Color color, float fontSize, TextAlign textAlign /*= TextAlign::LeftTop*/, TextBoxAlign boxAlign /*= TextBoxAlign::CenterCenter*/, bool useUnderline /*= false*/, bool useStrikeThrough /*= false*/)
{
	m_Context->CreateD2DRenderTarget(rt);
	m_Context->SetRTTransform(scale, posH);

	float left = posH.x;
	float top = posH.y;
	float right = left + size.x;
	float bottom = top + size.y;

	if (boxAlign == TextBoxAlign::CenterCenter)
	{
		left -= size.x * 0.5f;
		top -= size.y * 0.5f;
		right -= size.x * 0.5f;
		bottom -= size.y * 0.5f;
	}

	if (!font)
	{
		OutputDebugStringA("font is nullptr\n");
		return;
	}
	else
	{
		if (font->fontSize != fontSize)
		{
			font->fontSize = fontSize;
			m_Context->CreateTextFormat(font);
		}

		switch (textAlign)
		{
		case TextAlign::LeftTop:
			font->textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
			font->textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			break;
		case TextAlign::LeftCenter:
			font->textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
			font->textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			break;
		case TextAlign::LeftBottom:
			font->textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
			font->textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
			break;
		case TextAlign::CenterTop:
			font->textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			font->textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			break;
		case TextAlign::CenterCenter:
			font->textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			font->textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			break;
		case TextAlign::CenterBottom:
			font->textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			font->textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
			break;
		case TextAlign::RightTop:
			font->textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
			font->textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			break;
		case TextAlign::RightCenter:
			font->textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
			font->textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			break;
		case TextAlign::RightBottom:
			font->textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
			font->textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
			break;
		default:
			break;
		}

		m_Context->DrawText(font, text, left, top, right, bottom, color, useUnderline, useStrikeThrough);
	}
}

void NeoWooDXI::SubmitComboBox(Texture* rt, Font* font, const std::vector<std::string>& texts, uint32_t curIndex, bool isOpen, Vector2 posH, float leftPadding, Vector2 size, Vector2 scale, DirectX::SimpleMath::Color color, float fontSize, TextAlign textAlign /*= TextAlign::LeftTop*/, TextBoxAlign boxAlign /*= TextBoxAlign::CenterCenter*/, bool useUnderline /*= false*/)
{
	if (texts.empty())
		return;

	m_Context->CreateD2DRenderTarget(rt);
	m_Context->SetRTTransform(scale, posH);

	float left = posH.x;
	float top = posH.y;
	float right = left + size.x;
	float bottom = top + size.y;

	if (boxAlign == TextBoxAlign::CenterCenter)
	{
		left -= size.x * 0.5f;
		top -= size.y * 0.5f;
		right -= size.x * 0.5f;
		bottom -= size.y * 0.5f;
	}

	if (!font)
	{
		OutputDebugStringA("font is nullptr\n");
		return;
	}
	else
	{
		if (font->fontSize != fontSize)
		{
			font->fontSize = fontSize;
			m_Context->CreateTextFormat(font);
		}

		switch (textAlign)
		{
		case TextAlign::LeftTop:
			font->textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
			font->textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			break;
		case TextAlign::LeftCenter:
			font->textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
			font->textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			break;
		case TextAlign::LeftBottom:
			font->textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
			font->textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
			break;
		case TextAlign::CenterTop:
			font->textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			font->textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			break;
		case TextAlign::CenterCenter:
			font->textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			font->textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			break;
		case TextAlign::CenterBottom:
			font->textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			font->textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
			break;
		case TextAlign::RightTop:
			font->textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
			font->textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			break;
		case TextAlign::RightCenter:
			font->textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
			font->textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			break;
		case TextAlign::RightBottom:
			font->textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
			font->textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
			break;
		default:
			break;
		}

		m_Context->DrawComboBox(font, texts, curIndex, left, top, right, bottom, color, isOpen, leftPadding);
	}
}

void NeoWooDXI::LoadFontsFromDrive(const std::string& path)
{
	std::filesystem::path directory = path;

	if (!std::filesystem::exists(directory))
		return;

	if (std::filesystem::is_directory(directory))
		loadFontsFromDirectory(path);
	else
		loadFontsFromFile(path);
}

std::shared_ptr<Font> NeoWooDXI::GetFont(const std::string& name)
{
	if (m_Fonts.contains(name))
		return m_Fonts[name];

	return nullptr;
}

void NeoWooDXI::BeginDebugRender()
{
	auto context = m_Context->GetDeviceContext();

	context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(m_states->DepthNone(), 0);
	context->RSSetState(m_states->CullNone());

	m_effect->Apply(context);

	context->IASetInputLayout(m_inputLayout.Get());

	m_batch->Begin();

}

void NeoWooDXI::EndDebugRender()
{
	m_batch->End();
}

void NeoWooDXI::DrawBoundingBox(const DirectX::BoundingBox& box, const DirectX::SimpleMath::Vector4& color)
{
	DebugDraw::Draw(m_batch.get(), box, color);
}

void NeoWooDXI::DrawOrientedBoundingBox(const DirectX::BoundingOrientedBox& box, const DirectX::SimpleMath::Vector4& color)
{
	DebugDraw::Draw(m_batch.get(), box, color);
}

void NeoWooDXI::DrawBoundingSphere(const DirectX::BoundingSphere& sphere, const DirectX::SimpleMath::Vector4& color)
{
	DebugDraw::Draw(m_batch.get(), sphere, color);
}

void NeoWooDXI::bindShaderResources(Material& material)
{
	Shader* shader = material.m_Shader.get();

	DX11Shader* dx11Shader = static_cast<DX11Shader*>(shader);

	for (auto& samplerBinding : dx11Shader->m_SamplerBindings)
	{
		DX11Sampler* sampler = nullptr;

		switch (samplerBinding.samplerType)
		{
		case SamplerType::gsamLinearWrap:
		case SamplerType::gsamPointWrap:
		case SamplerType::gsamAnisotropicWrap:
			sampler = m_ResourceManager->CreateSampler(D3D11_TEXTURE_ADDRESS_WRAP, samplerBinding).get();
			break;

		case SamplerType::gsamPointClamp:
		case SamplerType::gsamLinearClamp:
		case SamplerType::gsamAnisotropicClamp:
			sampler = m_ResourceManager->CreateSampler(D3D11_TEXTURE_ADDRESS_CLAMP, samplerBinding).get();
			break;

		case SamplerType::gsamShadow:
			sampler = m_ResourceManager->CreateSampler(D3D11_TEXTURE_ADDRESS_BORDER, samplerBinding).get();
			break;

		default:
			break;
		}

		if (sampler)
		{
			sampler->SetShaderType(samplerBinding.Type);
			sampler->Bind(m_Context.get());
		}
		else
		{
			OutputDebugStringA("Sampler is not found\n");
		}
	}

	auto& textures = material.m_Textures;

	for (auto& binding : dx11Shader->m_TextureBindings)
	{
		auto it = textures.find(binding.Name);
		if (it != textures.end())
		{
			auto dx11Texture = std::static_pointer_cast<DX11Texture>(it->second.Texture);

			if (dx11Texture)
				dx11Texture->Bind(m_Context.get(), binding);
		}
		else
		{
			OutputDebugStringA(binding.Name.c_str());
			OutputDebugStringA(" : Binding Texture is not found\n");
		}
	}

	auto& uavTextures = material.m_UAVTextures;

	for (auto& binding : dx11Shader->m_UAVBindings)
	{
		auto it = uavTextures.find(binding.Name);
		if (it != uavTextures.end())
		{
			auto dx11Texture = std::static_pointer_cast<DX11Texture>(it->second);
			dx11Texture->BindUAV(m_Context.get(), binding);
		}
		else
			OutputDebugStringA("UAV Texture is not found\n");
	}
}

void NeoWooDXI::unbindShaderResources(Material& material)
{
	Shader* shader = material.m_Shader.get();

	DX11Shader* dx11Shader = static_cast<DX11Shader*>(shader);

	auto& textures = material.m_Textures;

	for (auto& binding : dx11Shader->m_TextureBindings)
	{
		auto it = textures.find(binding.Name);
		if (it != textures.end())
		{
			if (auto dx11Texture = std::static_pointer_cast<DX11Texture>(it->second.Texture))
				dx11Texture->Unbind(m_Context.get());
		}

	}


	auto& uavTextures = material.m_UAVTextures;


	for (auto& binding : dx11Shader->m_UAVBindings)
	{
		auto it = uavTextures.find(binding.Name);
		if (it != uavTextures.end())
		{
			auto dx11Texture = std::static_pointer_cast<DX11Texture>(it->second);
			dx11Texture->UnbindUAV(m_Context.get());
		}
	}
}

void NeoWooDXI::loadUITexturesFromDirectory(const std::string& path)
{
	std::filesystem::path directory(path);
	if (!std::filesystem::exists(directory))
		return;

	for (const auto& entry : std::filesystem::directory_iterator(directory))
	{
		if (entry.is_regular_file())
		{
			loadUITexturesFromFile(entry.path().string());
		}
	}
}

void NeoWooDXI::loadUITexturesFromFile(const std::string& path)
{
	std::shared_ptr<Texture> texture = Texture::Create(m_Context.get(), path, Texture::Type::Texture2D);

	if (texture == nullptr)
	{
		OutputDebugStringA("Failed to load texture\n");
		return;
	}

	std::string name = (static_pointer_cast<DX11Texture>(texture)->GetName());
	name = name.substr(name.find_last_of('\\') + 1);
	static_pointer_cast<DX11Texture>(texture)->SetName(name);

	m_UITextures[name] = texture;
}

void NeoWooDXI::loadParticleTexturesFromDirectory(const std::string& path)
{
	std::filesystem::path directory(path);
	if (!std::filesystem::exists(directory))
		return;

	for (const auto& entry : std::filesystem::directory_iterator(directory))
	{
		if (entry.is_regular_file())
		{
			loadParticleTexturesFromFile(entry.path().string());
		}
	}

}

void NeoWooDXI::loadParticleTexturesFromFile(const std::string& path)
{
	std::shared_ptr<Texture> texture = Texture::Create(m_Context.get(), path, Texture::Type::Texture2D);

	if (texture == nullptr)
	{
		OutputDebugStringA("Failed to load texture\n");
		return;
	}

	std::string name = (static_pointer_cast<DX11Texture>(texture)->GetName());
	name = name.substr(name.find_last_of('\\') + 1);
	static_pointer_cast<DX11Texture>(texture)->SetName(name);

	m_ParticleTextures[name] = texture;
}

void NeoWooDXI::loadFontsFromDirectory(const std::string& path)
{
	std::filesystem::path directory(path);
	if (!std::filesystem::exists(directory))
		return;

	for (const auto& entry : std::filesystem::directory_iterator(directory))
	{
		if (entry.is_regular_file())
		{
			loadFontsFromFile(entry.path().string());
		}
	}
}

void NeoWooDXI::loadFontsFromFile(const std::string& path)
{
	/*std::wstring wpath(path.begin(), path.end());
	int addedFonts = AddFontResourceEx(wpath.c_str(), FR_NOT_ENUM, nullptr);

	if (addedFonts > 0)
	{
		std::string fontName = std::filesystem::path(path).filename().replace_extension().string();
		std::shared_ptr<Font> font = std::make_shared<Font>(fontName);
		m_Context->CreateTextFormat(font.get());
		m_Fonts[fontName] = font;
	}*/

	std::shared_ptr<Font> font = std::make_shared<Font>();
	m_Context->CreateTextFormat(font.get(), std::filesystem::path(path));
	font->fontPath = path;

	std::string fileName = std::filesystem::path(path).filename().replace_extension().string();

	if (font->textFormat)
		m_Fonts[fileName] = font;


	/*std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		OutputDebugStringA("Failed to open file\n");
		return;
	}

	std::vector<BYTE> fontData((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	m_Context->CreateTextFormat(font.get(), fontData);
	m_Fonts[std::filesystem::path(path).filename().replace_extension().string()] = font;*/
}
