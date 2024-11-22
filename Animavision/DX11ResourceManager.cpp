#include "pch.h"
#include "DX11ResourceManager.h"
#include "DX11Buffer.h"
#include "DX11Shader.h"
#include "DX11Sampler.h"
#include "NeoDX11Context.h"
#include "DX11State.h"
#include <DirectXTex.h>
#include <DirectXTexEXR.h>

#include "../Animacore/ParticleStructures.h"

DX11ResourceManager::DX11ResourceManager(NeoDX11Context* context)
	: m_Context(context), m_RandomGen(m_RandomDevice())
{
	m_Samplers.resize(static_cast<uint32_t>(SamplerType::Count));
}

DX11ResourceManager::~DX11ResourceManager()
{
	m_TextureMap.clear();
	m_ShaderMap.clear();
	m_Samplers.clear();
	//m_SamplerMap.clear();
	m_DepthStencilStateMap.clear();
	m_BlendStateMap.clear();
	m_RasterizerStateMap.clear();
}

std::shared_ptr<DX11Texture> DX11ResourceManager::GetTexture(const std::string& name)
{
	auto findIt = m_TextureMap.find(name);
	if (findIt == m_TextureMap.end())
		return nullptr;

	return findIt->second;
}

std::shared_ptr<DX11Texture> DX11ResourceManager::CreateTexture(const std::string& path)
{
	auto it = m_TextureMap.find(path);
	if (it != m_TextureMap.end())
		return it->second;

	DirectX::ScratchImage image;
	DirectX::TexMetadata metadata = {};

	std::wstring wpath(path.begin(), path.end());
	//std::wstring extension = wpath.substr(wpath.find_last_of(L".") + 1);
	std::filesystem::path p = path;
	std::wstring extension = p.extension().wstring();
	extension = extension.substr(1);
	std::wstring fileName = wpath.substr(wpath.find_last_of('/') + 1);

	HRESULT hr = S_OK;

	// WOO : wic flags 필요하면 변경
	if (extension == L"png" || extension == L"jpg" || extension == L"jpeg" || extension == L"tiff" || extension == L"bmp")
		hr = DirectX::LoadFromWICFile(wpath.c_str(), DirectX::WIC_FLAGS_NONE, &metadata, image);

	// WOO : dds flags 필요하면 변경
	else if (extension == L"dds")
		hr = DirectX::LoadFromDDSFile(wpath.c_str(), DirectX::DDS_FLAGS_NONE, &metadata, image);

	else if (extension == L"tga")
		hr = DirectX::LoadFromTGAFile(wpath.c_str(), &metadata, image);

	else if (extension == L"hdr")
		hr = DirectX::LoadFromHDRFile(wpath.c_str(), &metadata, image);

	auto texture = std::make_shared<DX11Texture>(m_Context, path, metadata, image);
	m_TextureMap[path] = texture;

	return texture;
}

std::shared_ptr<DX11Texture> DX11ResourceManager::CreateEmptyTexture(std::string name, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, Texture::Format format, Texture::Usage usage, float* clearColor, Texture::UAVType uavType, uint32_t arraySize)
{
	std::shared_ptr<DX11Texture> texture = std::make_shared<DX11Texture>(m_Context, name, type, width, height, mipLevels, format, usage, clearColor, uavType, arraySize);

	m_TextureMap.insert({ name, texture });

	return texture;
}

std::shared_ptr<DX11Texture> DX11ResourceManager::CreateEmptyTexture(const TextureDesc& desc)
{
	std::shared_ptr<DX11Texture> texture = std::make_shared<DX11Texture>(m_Context, desc);

	m_TextureMap.insert({ desc.name, texture });

	return texture;
}

std::shared_ptr<DX11Texture> DX11ResourceManager::CreateTexture2D(const std::string& path)
{
	auto it = m_TextureMap.find(path);
	if (it != m_TextureMap.end())
		return it->second;

	auto texture = std::make_shared<DX11Texture>(m_Context, path, Texture::Type::Texture2D);
	m_TextureMap[path] = texture;

	return texture;
}

std::shared_ptr<DX11Texture> DX11ResourceManager::CreateLightmapTexture(const std::string& path)
{
	std::filesystem::path currentPath = std::filesystem::current_path();
	currentPath = currentPath / "Resources/";
	std::string pathString = currentPath.string() + path;
	const std::wstring& wpath = std::wstring(pathString.begin(), pathString.end());
	//std::wstring extension = wpath.substr(wpath.find_last_of(L".") + 1);

	std::filesystem::path p = path;
	std::wstring extension = p.extension().wstring();
	extension = extension.substr(1);
	ID3D11Texture2D* texture = nullptr;

	if (!std::filesystem::exists(wpath))
		OutputDebugStringA("file not exist\n");

	HRESULT hr = S_OK;

	if (extension == L"png" || extension == L"jpg" || extension == L"jpeg" || extension == L"tiff" || extension == L"bmp")
		hr = DirectX::CreateWICTextureFromFileEx(
			m_Context->GetDevice(),
			wpath.c_str(),
			0,
			D3D11_USAGE_STAGING,
			0,
			D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ,
			0,
			DirectX::DX11::WIC_LOADER_DEFAULT,
			reinterpret_cast<ID3D11Resource**>(&texture),
			nullptr
		);

	// WOO : dds flags 필요하면 변경
	else if (extension == L"dds")
		hr = DirectX::CreateDDSTextureFromFileEx(
			m_Context->GetDevice(),
			wpath.c_str(),
			0,
			D3D11_USAGE_STAGING,
			0,
			D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ,
			0,
			DirectX::DDS_LOADER_DEFAULT,
			reinterpret_cast<ID3D11Resource**>(&texture),
			nullptr
		);

	// unity light map이 exr 파일로 나온다
	else if (extension == L"exr")
	{
		DirectX::ScratchImage image;
		hr = DirectX::LoadFromEXRFile(wpath.c_str(), nullptr, image);
		if (SUCCEEDED(hr))
		{
			DirectX::TexMetadata metadata = image.GetMetadata();

			if (metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE2D)
			{
				DirectX::ScratchImage mipChain;
				hr = GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_DEFAULT, 0, mipChain);
				if (SUCCEEDED(hr))
				{
					hr = DirectX::CreateTextureEx(
						m_Context->GetDevice(),
						mipChain.GetImages(),
						mipChain.GetImageCount(),
						mipChain.GetMetadata(),
						D3D11_USAGE_STAGING,
						0,
						D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ,
						0,
						DirectX::CREATETEX_FLAGS::CREATETEX_DEFAULT,
						reinterpret_cast<ID3D11Resource**>(&texture)
					);
				}
			}
		}
	}

	else
		assert(false && "Invalid texture extension");

	std::shared_ptr<DX11Texture> lightmapTexture = std::make_shared<DX11Texture>();
	lightmapTexture->SetTexture2D(texture);

	return lightmapTexture;
}

std::shared_ptr<DX11Texture> DX11ResourceManager::CreateTexture3D(const std::string& path)
{
	auto it = m_TextureMap.find(path);
	if (it != m_TextureMap.end())
		return it->second;

	auto texture = std::make_shared<DX11Texture>(m_Context, path, Texture::Type::Texture3D);
	m_TextureMap[path] = texture;

	return texture;

}

std::shared_ptr<DX11Texture> DX11ResourceManager::CreateTexture2DArray(const std::string& path)
{
	auto it = m_TextureMap.find(path);
	if (it != m_TextureMap.end())
		return it->second;

	auto texture = std::make_shared<DX11Texture>(m_Context, path, Texture::Type::Texture2DArray);
	m_TextureMap[path] = texture;

	return texture;
}

std::shared_ptr<DX11Texture> DX11ResourceManager::CreateTextureCube(const std::string& path)
{
	auto it = m_TextureMap.find(path);
	if (it != m_TextureMap.end())
		return it->second;

	auto texture = std::make_shared<DX11Texture>(m_Context, path, Texture::Type::TextureCube);
	m_TextureMap[path] = texture;

	return texture;
}

std::shared_ptr<DX11Shader> DX11ResourceManager::CreateShader(const std::string& path)
{
	auto it = m_ShaderMap.find(path);
	if (it != m_ShaderMap.end())
		return it->second;

	auto shader = std::make_shared<DX11Shader>(m_Context, path);
	m_ShaderMap[path] = shader;

	return shader;
}

std::shared_ptr<DX11Sampler> DX11ResourceManager::CreateSampler(D3D11_TEXTURE_ADDRESS_MODE addressMode, const DX11ResourceBinding& samplerBinding)
{
	//std::ostringstream oss;
	//oss << samplerBinding.Name /*<< samplerBinding.Register*/;
	//auto&& name = oss.str();

	auto findIt = m_Samplers[static_cast<uint32_t>(samplerBinding.samplerType)];

	if (findIt != nullptr)
	{
		findIt->SetSlot(samplerBinding.Register);
		return findIt;
	}

	auto sampler = std::make_shared<DX11Sampler>(m_Context, addressMode, samplerBinding);
	m_Samplers[static_cast<uint32_t>(samplerBinding.samplerType)] = sampler;

	return sampler;
}

std::shared_ptr<DX11DepthStencilState> DX11ResourceManager::CreateDepthStencilState(DepthStencilState state)
{
	if (state == DepthStencilState::COUNT)
		return nullptr;

	auto findIt = m_DepthStencilStateMap.find(state);
	if (findIt != m_DepthStencilStateMap.end())
		return findIt->second;

	auto depthStencilState = std::make_shared<DX11DepthStencilState>(*m_Context, state);
	m_DepthStencilStateMap[state] = depthStencilState;
	return depthStencilState;
}

std::shared_ptr<DX11BlendState> DX11ResourceManager::CreateBlendState(BlendState state)
{
	if (state == BlendState::COUNT)
		return nullptr;

	auto findIt = m_BlendStateMap.find(state);
	if (findIt != m_BlendStateMap.end())
		return findIt->second;

	auto blendState = std::make_shared<DX11BlendState>(*m_Context, state);
	m_BlendStateMap[state] = blendState;
	return blendState;
}

std::shared_ptr<DX11RasterizerState> DX11ResourceManager::CreateRasterizerState(RasterizerState state)
{
	if (state == RasterizerState::COUNT)
		return nullptr;

	auto findIt = m_RasterizerStateMap.find(state);
	if (findIt != m_RasterizerStateMap.end())
		return findIt->second;

	auto rasterizerState = std::make_shared<DX11RasterizerState>(*m_Context, state);
	m_RasterizerStateMap[state] = rasterizerState;
	return rasterizerState;
}

void DX11ResourceManager::ReleaseTexture(const std::string& name)
{
	auto findIt = m_TextureMap.find(name);
	if (findIt == m_TextureMap.end())
		return;

	m_TextureMap.erase(findIt);
}

std::shared_ptr<DX11Texture> DX11ResourceManager::CreateIndirectDrawTexture()
{
	if (m_TextureMap.contains("IndirectDrawTexture"))
		return m_TextureMap["IndirectDrawTexture"];

	D3D11_BUFFER_DESC desc = {};

	// 간접 호출을 위한 상수 버퍼와 UAV
	ZeroMemory(&desc, sizeof(desc));
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	desc.ByteWidth = 5 * sizeof(uint32_t);
	desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;

	std::shared_ptr<DX11Texture> returnTexture = std::make_shared<DX11Texture>();

	HRESULT hr = m_Context->GetDevice()->CreateBuffer(&desc, nullptr, returnTexture->GetBufferAddress());

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	ZeroMemory(&uavDesc, sizeof(uavDesc));
	uavDesc.Format = DXGI_FORMAT_R32_UINT;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = 5;
	uavDesc.Buffer.Flags = 0;
	hr = m_Context->GetDevice()->CreateUnorderedAccessView(returnTexture->GetBuffer(), &uavDesc, returnTexture->GetUAVAddress());

	m_TextureMap.insert({ "IndirectDrawTexture", returnTexture });
	return returnTexture;
}

ParticleBuffers DX11ResourceManager::CreateParticleBuffers(uint32_t maxCount, uint32_t entity)
{
	ParticleBuffers returnBuffers;

	std::string key0 = "ParticleResource" + std::to_string(entity) + std::to_string(0);
	std::string key1 = "ParticleResource" + std::to_string(entity) + std::to_string(1);
	std::string key2 = "ParticleResource" + std::to_string(entity) + std::to_string(2);

	if (m_TextureMap.contains(key0) && m_TextureMap.contains(key1) && m_TextureMap.contains(key2))
	{
		returnBuffers = { m_TextureMap[key0], m_TextureMap[key1], m_TextureMap[key2] };		
		return returnBuffers;
	}

	std::shared_ptr<DX11Texture> particleBuffer = std::make_shared<DX11Texture>();

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.ByteWidth = sizeof(core::Particle) * maxCount;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bufferDesc.StructureByteStride = sizeof(core::Particle);

	ThrowIfFailed(m_Context->GetDevice()->CreateBuffer(&bufferDesc, nullptr, particleBuffer->GetBufferAddress()));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.ElementOffset = 0;
	srvDesc.Buffer.ElementWidth = maxCount;

	ThrowIfFailed(m_Context->GetDevice()->CreateShaderResourceView(particleBuffer->GetBuffer(), &srvDesc, particleBuffer->GetSRVAddress()));

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = maxCount;
	uavDesc.Buffer.Flags = 0;

	ThrowIfFailed(m_Context->GetDevice()->CreateUnorderedAccessView(particleBuffer->GetBuffer(), &uavDesc, particleBuffer->GetUAVAddress()));
	particleBuffer->m_Type = Texture::Type::Buffer;
	m_TextureMap.insert({ key0, particleBuffer });

	std::shared_ptr<DX11Texture> deadListBuffer = std::make_shared<DX11Texture>();

	bufferDesc.ByteWidth = sizeof(uint32_t) * maxCount;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bufferDesc.StructureByteStride = sizeof(uint32_t);

	ThrowIfFailed(m_Context->GetDevice()->CreateBuffer(&bufferDesc, nullptr, deadListBuffer->GetBufferAddress()));

	uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;

	ThrowIfFailed(m_Context->GetDevice()->CreateUnorderedAccessView(deadListBuffer->GetBuffer(), &uavDesc, deadListBuffer->GetUAVAddress()));
	deadListBuffer->m_Type = Texture::Type::Buffer;
	m_TextureMap.insert({ key1, deadListBuffer });

	std::shared_ptr<DX11Texture> aliveListBuffer = std::make_shared<DX11Texture>();

	bufferDesc.ByteWidth = sizeof(uint32_t) * maxCount;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bufferDesc.StructureByteStride = sizeof(uint32_t);

	ThrowIfFailed(m_Context->GetDevice()->CreateBuffer(&bufferDesc, nullptr, aliveListBuffer->GetBufferAddress()));

	srvDesc.Buffer.ElementWidth = maxCount;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.ElementOffset = 0;

	ThrowIfFailed(m_Context->GetDevice()->CreateShaderResourceView(aliveListBuffer->GetBuffer(), &srvDesc, aliveListBuffer->GetSRVAddress()));

	uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_COUNTER;
	uavDesc.Buffer.NumElements = maxCount;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;

	ThrowIfFailed(m_Context->GetDevice()->CreateUnorderedAccessView(aliveListBuffer->GetBuffer(), &uavDesc, aliveListBuffer->GetUAVAddress()));
	aliveListBuffer->m_Type = Texture::Type::Buffer;
	m_TextureMap.insert({ key2, aliveListBuffer });

	returnBuffers = { m_TextureMap[key0], m_TextureMap[key1], m_TextureMap[key2] };
	return returnBuffers;
}

float DX11ResourceManager::GetRandomSeed(float min, float max)
{
	return std::uniform_real_distribution<float>(min, max)(m_RandomGen);
}

std::shared_ptr<DX11Texture> DX11ResourceManager::CreateRandomTexture()
{
	if (m_TextureMap.contains("RandomTexture"))
		return m_TextureMap["RandomTexture"];

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = 1024;
	desc.Height = 1024;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

	float* values = new float[desc.Width * desc.Height * 4];
	float* ptr = values;
	for (uint32_t i = 0; i < desc.Width * desc.Height; i++)
	{
		ptr[0] = dist(m_RandomGen);
		ptr[1] = dist(m_RandomGen);
		ptr[2] = dist(m_RandomGen);
		ptr[3] = dist(m_RandomGen);
		ptr += 4;
	}

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = values;
	initData.SysMemPitch = desc.Width * 16;
	initData.SysMemSlicePitch = 0;

	std::shared_ptr<DX11Texture> returnTexture = std::make_shared<DX11Texture>();

	ThrowIfFailed(m_Context->GetDevice()->CreateTexture2D(&desc, &initData, returnTexture->GetTexture2DAddress()));

	delete[] values;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	ThrowIfFailed(m_Context->GetDevice()->CreateShaderResourceView(returnTexture->GetTexture2D(), &srvDesc, returnTexture->GetSRVAddress()));

	returnTexture->m_Type = Texture::Type::Texture2D;

	m_TextureMap.insert({ "RandomTexture", returnTexture });
	return returnTexture;
}

