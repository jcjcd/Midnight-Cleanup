#pragma once

#include "ShaderResource.h"

class DX12Buffer;
class DX12Texture;
class SingleDescriptorAllocator;
class CommandObjectPool;
struct TextureDesc;

class DX12ResourceManager
{
public:
	DX12ResourceManager(winrt::com_ptr<ID3D12Device5> device, std::shared_ptr<CommandObjectPool> commandObjectPool, bool isRaytracing);
	~DX12ResourceManager();

	std::shared_ptr<DX12Buffer> CreateVertexBuffer(const void* data, uint32_t count, uint32_t stride);
	std::shared_ptr<DX12Buffer> CreateIndexBuffer(const void* data, uint32_t count, uint32_t stride);

	std::shared_ptr<DX12Texture> GetTexture(const std::string& path);
	std::shared_ptr<DX12Texture> CreateTexture(const std::string& path);
	std::shared_ptr<DX12Texture> CreateTextureCube(const std::string& path);

	// buffer에서는 width가 stride, height가 개수이다. 

	std::shared_ptr<DX12Texture> CreateEmptyTexture(std::string name, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, Texture::Format format, Texture::Usage usage = Texture::Usage::NONE, float* clearColor = nullptr, uint32_t arraySize = 1);
	std::shared_ptr<DX12Texture> CreateEmptyTexture(const TextureDesc& desc);
	std::shared_ptr<DX12Texture> CreateEmplyBuffer(std::string name, uint32_t byteStride, uint32_t count, bool isUAV) { return CreateEmptyTexture(name, Texture::Type::Buffer, byteStride, count, 1, Texture::Format::UNKNOWN, Texture::Usage::NONE, nullptr); }
	void ReleaseTexture(const std::string& name);

	void CreateBufferSRV(DX12Buffer* buffer);

private:
	HRESULT CreateBufferBase(DX12Buffer* inoutBuffer, const void* data, uint32_t count, uint32_t stride);
	void UpdateTexturesForWrite(ID3D12Resource* pDestTexResource, ID3D12Resource* pSrcTexResource);

private:
	winrt::com_ptr<ID3D12Device5> m_Device = nullptr;					// 디바이스

	std::shared_ptr<CommandObjectPool> m_CommandObjectPool;

	bool m_RayTracing = false;

	// 리소스 관리
	std::shared_ptr<SingleDescriptorAllocator> m_RTVAllocator;
	std::shared_ptr<SingleDescriptorAllocator> m_DSVAllocator;
	std::shared_ptr<SingleDescriptorAllocator> m_DescriptorAllocator;
	robin_hood::unordered_map<std::string, std::shared_ptr<DX12Texture>> m_TextureMap;

	friend class ChangDXII;
};

