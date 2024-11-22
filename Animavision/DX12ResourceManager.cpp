#include "pch.h"
#include "DX12ResourceManager.h"
#include "DX12Helper.h"
#include "DX12Buffer.h"
#include "DX12Texture.h"
#include "DirectXTex.h"
#include "Descriptor.h"
#include "CommandObject.h"
#include "Renderer.h"

DX12ResourceManager::DX12ResourceManager(winrt::com_ptr<ID3D12Device5> device, std::shared_ptr<CommandObjectPool> commandObjectPool, bool isRaytracing)
{
	m_Device = device;
	m_CommandObjectPool = commandObjectPool;

	m_RayTracing = isRaytracing;

	//m_DescriptorAllocator = std::make_shared<SingleDescriptorAllocator>(m_Device, 1024)
	m_DescriptorAllocator = std::make_shared<SingleDescriptorAllocator>(m_Device, 4096,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

	m_RTVAllocator = std::make_shared<SingleDescriptorAllocator>(m_Device, 1024,
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	m_DSVAllocator = std::make_shared<SingleDescriptorAllocator>(m_Device, 1024,
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

}

DX12ResourceManager::~DX12ResourceManager()
{

}

std::shared_ptr<DX12Buffer> DX12ResourceManager::CreateVertexBuffer(const void* data, uint32_t count, uint32_t stride)
{
	HRESULT hr = S_OK;

	std::shared_ptr<DX12VertexBuffer> buffer = std::make_shared<DX12VertexBuffer>();

	uint32_t size = count * stride;

	hr = CreateBufferBase(buffer.get(), data, count, stride);

	if (FAILED(hr))
	{
		OutputDebugString(L"버텍스 버퍼 생성 실패");
		return nullptr;
	}

	buffer->m_Stride = stride;
	buffer->m_Count = count;
	buffer->m_VertexBufferView.BufferLocation = buffer->m_Buffer->GetGPUVirtualAddress();
	buffer->m_VertexBufferView.StrideInBytes = stride;
	buffer->m_VertexBufferView.SizeInBytes = size;

	CreateBufferSRV(buffer.get());

	return buffer;
}

std::shared_ptr<DX12Buffer> DX12ResourceManager::CreateIndexBuffer(const void* data, uint32_t count, uint32_t stride)
{
	HRESULT hr = S_OK;

	std::shared_ptr<DX12IndexBuffer> buffer = std::make_shared<DX12IndexBuffer>();

	uint32_t size = count * stride;

	ThrowIfFailed(CreateBufferBase(buffer.get(), data, count, stride));

	buffer->m_Count = count;
	buffer->m_IndexBufferView.BufferLocation = buffer->m_Buffer->GetGPUVirtualAddress();
	switch (stride)
	{
	case 2:
		buffer->m_IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
		break;
	case 4:
		buffer->m_IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
		break;
	default:
		break;
	}
	buffer->m_IndexBufferView.SizeInBytes = size;

	CreateBufferSRV(buffer.get());

	return buffer;
}

std::shared_ptr<DX12Texture> DX12ResourceManager::GetTexture(const std::string& path)
{
	auto it = m_TextureMap.find(path);
	if (it != m_TextureMap.end())
	{
		return it->second;
	}

	return nullptr;
}

// 창 : 이거 밉맵다 해주기.. 
// 오히려 ddsLoader로 해버릴까?

std::shared_ptr<DX12Texture> DX12ResourceManager::CreateTexture(const std::string& path)
{
	auto it = m_TextureMap.find(path);
	if (it != m_TextureMap.end())
	{
		return it->second;
	}

	std::shared_ptr<DX12Texture> texture = std::make_shared<DX12Texture>();

	if (path.empty() || path == "\"\"")
	{
		OutputDebugString(L"빈 텍스쳐 경로 생성 불가");
		return nullptr;
	}

	texture->m_Path = path;

	DirectX::ScratchImage image;
	DirectX::TexMetadata metadata{};

	std::wstring wpath(path.begin(), path.end());
	std::wstring extension = wpath.substr(wpath.find_last_of(L".") + 1);
	std::wstring fileName = wpath.substr(path.find_last_of('/') + 1);

	HRESULT hr = S_OK;

	if (extension == L"png" || extension == L"jpg" || extension == L"jpeg" || extension == L"tiff" || extension == L"bmp")
		hr = DirectX::LoadFromWICFile(wpath.c_str(), DirectX::WIC_FLAGS_NONE, &metadata, image);

	else if (extension == L"dds")
		hr = DirectX::LoadFromDDSFile(wpath.c_str(), DirectX::DDS_FLAGS_NONE, &metadata, image);

	else if (extension == L"tga")
		hr = DirectX::LoadFromTGAFile(wpath.c_str(), &metadata, image);

	else if (extension == L"hdr")
		hr = DirectX::LoadFromHDRFile(wpath.c_str(), &metadata, image);

	if (FAILED(hr))
	{
		OutputDebugString(L"텍스쳐 로드 실패");
		return nullptr;
	}

	// 텍스처가 큐브 맵인지 확인하고 타입 설정
	if (metadata.IsCubemap())
	{
		texture->m_Type = Texture::Type::TextureCube;
	}
	else
	{
		texture->m_Type = Texture::Type::Texture2D;
	}

	texture->m_Width = (uint32_t)metadata.width;
	texture->m_Height = (uint32_t)metadata.height;
	texture->m_UsageState = D3D12_RESOURCE_STATE_COPY_DEST;

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = (UINT16)metadata.mipLevels;
	textureDesc.Format = metadata.format;
	textureDesc.Width = texture->m_Width;
	textureDesc.Height = texture->m_Height;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = metadata.IsCubemap() ? (UINT16)metadata.arraySize : 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	ThrowIfFailed(m_Device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		texture->m_UsageState,
		nullptr,
		IID_PPV_ARGS(&texture->m_Texture)));

	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture->m_Texture.get(), 0, UINT(metadata.mipLevels * metadata.arraySize));
	winrt::com_ptr<ID3D12Resource> textureUploadHeap;

	ThrowIfFailed(m_Device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&textureUploadHeap)));

	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	subresources.reserve(metadata.mipLevels * metadata.arraySize);

	for (size_t i = 0; i < metadata.arraySize; ++i)
	{
		for (size_t j = 0; j < metadata.mipLevels; ++j)
		{
			const DirectX::Image* img = image.GetImage(j, i, 0);

			D3D12_SUBRESOURCE_DATA subresource = {};
			subresource.pData = img->pixels;
			subresource.RowPitch = img->rowPitch;
			subresource.SlicePitch = img->slicePitch;
			subresources.push_back(subresource);
		}
	}

	auto commandObject = m_CommandObjectPool->QueryCommandObject(D3D12_COMMAND_LIST_TYPE_DIRECT);
	ID3D12GraphicsCommandList* commandList = commandObject->GetCommandList();

	UpdateSubresources(commandList, texture->m_Texture.get(), textureUploadHeap.get(), 0, 0, (UINT)subresources.size(), subresources.data());

	texture->ResourceBarrier(commandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

	auto srvDescriptor = m_DescriptorAllocator->Allocate();
	texture->m_CpuDescriptorHandle = srvDescriptor;
	texture->m_GpuDescriptorHandle = m_DescriptorAllocator->GetGPUHandleFromCPUHandle(srvDescriptor);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = metadata.IsCubemap() ? D3D12_SRV_DIMENSION_TEXTURECUBE : D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.TextureCube.MipLevels = srvDesc.Texture2D.MipLevels = (UINT)metadata.mipLevels;
	srvDesc.TextureCube.MostDetailedMip = srvDesc.Texture2D.MostDetailedMip = 0;

	m_Device->CreateShaderResourceView(texture->m_Texture.get(), &srvDesc, srvDescriptor);

	commandObject->Finish(true);

	m_TextureMap.insert({ path, texture });

	return texture;
}


std::shared_ptr<DX12Texture> DX12ResourceManager::CreateTextureCube(const std::string& path)
{
	return CreateTexture(path);

}


std::shared_ptr<DX12Texture> DX12ResourceManager::CreateEmptyTexture(std::string name, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, Texture::Format format, Texture::Usage usage /*= Texture::Usage::NONE*/, float* clearColor /*= nullptr*/, uint32_t arraySize /*= 1*/)
{
	TextureDesc desc(name, type, width, height, mipLevels, format, usage, clearColor, Texture::UAVType::NONE, arraySize);
	return CreateEmptyTexture(desc);
}

std::shared_ptr<DX12Texture> DX12ResourceManager::CreateEmptyTexture(const TextureDesc& desc)
{
	std::shared_ptr<DX12Texture> texture = std::make_shared<DX12Texture>();

	texture->m_Path = desc.name;
	texture->m_Type = desc.type;

	auto& device = m_Device;

	// 이거 레퍼런스
	//CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);


	texture->m_Width = desc.width;
	texture->m_Height = desc.height;

	texture->m_UsageState = D3D12_RESOURCE_STATE_COMMON;

	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
	HasFlag(desc.usage, Texture::Usage::RTV) ? flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : flags;
	HasFlag(desc.usage, Texture::Usage::UAV) ? flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : flags;
	HasFlag(desc.usage, Texture::Usage::DSV) ? flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : flags;

	// 	isRenderTarget ? flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : flags;
	// 	isUAV ? flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : flags;

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = desc.mipLevels;
	textureDesc.Format = (DXGI_FORMAT)desc.format;
	textureDesc.Width = desc.width;
	textureDesc.Height = desc.height;
	textureDesc.Flags = flags;
	// 이 부분도 지원해야한다.
	textureDesc.DepthOrArraySize = desc.arraySize;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	switch (desc.type)
	{
	case Texture::Type::Unknown:
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_UNKNOWN;
		break;
	case Texture::Type::Buffer:
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		textureDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		// 버퍼일때는 width가 바이트 수이다.
		// |*|*|..|*|
		// 이 구조가
		// |*|
		// |*|
		// ..
		// |*|
		// 이렇게 되었다고생각하자
		textureDesc.Width = desc.width * desc.height;
		textureDesc.Height = 1;
		break;
	case Texture::Type::Texture1D:
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		break;
	case Texture::Type::Texture1DArray:
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		break;
	case Texture::Type::Texture2D:
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		break;
	case Texture::Type::Texture2DArray:
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		break;
	case Texture::Type::Texture2DMS:
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		break;
	case Texture::Type::Texture2DMSArray:
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		break;
	case Texture::Type::Texture3D:
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
		break;
	case Texture::Type::TextureCube:
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		break;
	case Texture::Type::TextureCubeArray:
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		break;
	case Texture::Type::COUNT:
	default:
		break;
	}

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = textureDesc.Format;
	if (desc.clearColor)
	{
		clearValue.Color[0] = desc.clearColor[0];
		clearValue.Color[1] = desc.clearColor[1];
		clearValue.Color[2] = desc.clearColor[2];
		clearValue.Color[3] = desc.clearColor[3];

		texture->m_ClearValue[0] = desc.clearColor[0];
		texture->m_ClearValue[1] = desc.clearColor[1];
		texture->m_ClearValue[2] = desc.clearColor[2];
		texture->m_ClearValue[3] = desc.clearColor[3];
	}
	else
	{
		clearValue.Color[0] = 0.0f;
		clearValue.Color[1] = 0.0f;
		clearValue.Color[2] = 0.0f;
		clearValue.Color[3] = 1.0f;

		texture->m_ClearValue[0] = 0.0f;
		texture->m_ClearValue[1] = 0.0f;
		texture->m_ClearValue[2] = 0.0f;
		texture->m_ClearValue[3] = 1.0f;
	}

	D3D12_CLEAR_VALUE* pClearValue = desc.clearColor ? &clearValue : nullptr;

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		texture->m_UsageState,
		pClearValue,
		IID_PPV_ARGS(&texture->m_Texture)));

	auto srvDescriptor = m_DescriptorAllocator->Allocate();
	texture->m_CpuDescriptorHandle = srvDescriptor;
	texture->m_GpuDescriptorHandle = m_DescriptorAllocator->GetGPUHandleFromCPUHandle(srvDescriptor);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = (DXGI_FORMAT)desc.srvFormat;
	srvDesc.ViewDimension = (D3D12_SRV_DIMENSION)desc.type;

	switch (desc.type)
	{
	case Texture::Type::Buffer:
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.StructureByteStride = desc.width;
		srvDesc.Buffer.NumElements = desc.height;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		break;
	case Texture::Type::Texture1D:
		srvDesc.Texture1D.MipLevels = desc.mipLevels;
		srvDesc.Texture1D.MostDetailedMip = 0;
		srvDesc.Texture1D.ResourceMinLODClamp = 0.0f;
		break;
	case Texture::Type::Texture1DArray:
		srvDesc.Texture1DArray.MipLevels = desc.mipLevels;
		srvDesc.Texture1DArray.MostDetailedMip = 0;
		srvDesc.Texture1DArray.ResourceMinLODClamp = 0.0f;
		srvDesc.Texture1DArray.FirstArraySlice = 0;
		srvDesc.Texture1DArray.ArraySize = desc.arraySize;
		break;
	case Texture::Type::Texture2D:
		srvDesc.Texture2D.MipLevels = desc.mipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		break;
	case Texture::Type::Texture2DArray:
		srvDesc.Texture2DArray.MipLevels = desc.mipLevels;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.ArraySize = desc.arraySize;
		break;
	case Texture::Type::Texture2DMS:
		srvDesc.Texture2DMS.UnusedField_NothingToDefine;
		break;
	case Texture::Type::Texture2DMSArray:
		srvDesc.Texture2DMSArray.FirstArraySlice = 0;
		srvDesc.Texture2DMSArray.ArraySize = desc.arraySize;
		break;
	case Texture::Type::Texture3D:
		srvDesc.Texture3D.MipLevels = desc.mipLevels;
		srvDesc.Texture3D.MostDetailedMip = 0;
		srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
		break;
	case Texture::Type::TextureCube:
		srvDesc.TextureCube.MipLevels = desc.mipLevels;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		break;
	case Texture::Type::TextureCubeArray:
		srvDesc.TextureCubeArray.MipLevels = desc.mipLevels;
		srvDesc.TextureCubeArray.MostDetailedMip = 0;
		srvDesc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
		srvDesc.TextureCubeArray.First2DArrayFace = 0;
		srvDesc.TextureCubeArray.NumCubes = 1;
		break;
	default:
		break;
	}

	device->CreateShaderResourceView(texture->m_Texture.get(), &srvDesc, srvDescriptor);





	if (HasFlag(desc.usage, Texture::Usage::RTV))
	{
		auto rtvDescriptor = m_RTVAllocator->Allocate();
		texture->m_CpuDescriptorHandleRTV = rtvDescriptor;

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = (DXGI_FORMAT)desc.rtvFormat;
		rtvDesc.ViewDimension = (D3D12_RTV_DIMENSION)desc.type;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;

		device->CreateRenderTargetView(texture->m_Texture.get(), &rtvDesc, rtvDescriptor);
	}

	if (HasFlag(desc.usage, Texture::Usage::UAV))
	{
		auto uavDescriptor = m_DescriptorAllocator->Allocate();
		texture->m_CpuDescriptorHandleUAV = uavDescriptor;
		texture->m_GpuDescriptorHandleUAV = m_DescriptorAllocator->GetGPUHandleFromCPUHandle(uavDescriptor);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = (DXGI_FORMAT)desc.uavFormat;
		uavDesc.ViewDimension = (D3D12_UAV_DIMENSION)desc.type;

		switch (desc.type)
		{
		case Texture::Type::Buffer:
			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.NumElements = desc.width;
			uavDesc.Buffer.StructureByteStride = desc.height;
			uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
			break;
		case Texture::Type::Texture1D:
			uavDesc.Texture1D.MipSlice = 0;
			break;
		case Texture::Type::Texture1DArray:
			uavDesc.Texture1DArray.MipSlice = 0;
			uavDesc.Texture1DArray.FirstArraySlice = 0;
			uavDesc.Texture1DArray.ArraySize = desc.arraySize;
			break;
		case Texture::Type::Texture2D:
			uavDesc.Texture2D.MipSlice = 0;
			uavDesc.Texture2D.PlaneSlice = 0;
			break;
		case Texture::Type::Texture2DArray:
			uavDesc.Texture2DArray.MipSlice = 0;
			uavDesc.Texture2DArray.FirstArraySlice = 0;
			uavDesc.Texture2DArray.ArraySize = desc.arraySize;
			break;
		case Texture::Type::Texture3D:
			uavDesc.Texture3D.MipSlice = 0;
			uavDesc.Texture3D.FirstWSlice = 0;
			uavDesc.Texture3D.WSize = 1;
			break;
		default:
			break;
		}

		device->CreateUnorderedAccessView(texture->m_Texture.get(), nullptr, &uavDesc, uavDescriptor);
	}

	if (HasFlag(desc.usage, Texture::Usage::DSV))
	{
		auto dsvDescriptor = m_DSVAllocator->Allocate();
		texture->m_CpuDescriptorHandleDSV = dsvDescriptor;

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = (DXGI_FORMAT)desc.dsvFormat;
		dsvDesc.ViewDimension = (D3D12_DSV_DIMENSION)desc.type;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		switch (desc.type)
		{
		case Texture::Type::Texture2D:
			dsvDesc.Texture2D.MipSlice = 0;
			dsvDesc.Texture2DArray.FirstArraySlice = 0;
			dsvDesc.Texture2DArray.ArraySize = desc.arraySize;
			break;
		case Texture::Type::Texture2DArray:
			dsvDesc.Texture2DArray.MipSlice = 0;
			dsvDesc.Texture2DArray.FirstArraySlice = 0;
			dsvDesc.Texture2DArray.ArraySize = desc.arraySize;
			break;
		}

		device->CreateDepthStencilView(texture->m_Texture.get(), &dsvDesc, dsvDescriptor);
	}

	texture->m_Texture->SetName(std::wstring(desc.name.begin(), desc.name.end()).c_str());


	m_TextureMap.insert({ desc.name, texture });

	return texture;

}

void DX12ResourceManager::ReleaseTexture(const std::string& name)
{
	auto it = m_TextureMap.find(name);
	if (it != m_TextureMap.end())
	{
		m_TextureMap.erase(it);
	}
}

// 창 : 여기 확인해볼것
void DX12ResourceManager::CreateBufferSRV(DX12Buffer* buffer)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;

	if (buffer->m_Stride == 0)
	{
		srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
	}
	else
	{
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	}

	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.StructureByteStride = buffer->m_Stride;
	srvDesc.Buffer.NumElements = buffer->m_Count;


	auto srvDescriptor = m_DescriptorAllocator->Allocate();
	buffer->m_SRVCpuHandle = srvDescriptor;
	buffer->m_SRVGpuHandle = m_DescriptorAllocator->GetGPUHandleFromCPUHandle(srvDescriptor);

	m_Device->CreateShaderResourceView(buffer->m_Buffer.get(), &srvDesc, buffer->m_SRVCpuHandle);
}

HRESULT DX12ResourceManager::CreateBufferBase(DX12Buffer* inoutBuffer, const void* data, uint32_t count, uint32_t stride)
{
	HRESULT hr = S_OK;

	winrt::com_ptr<ID3D12Resource> uploadBuffer;

	const volatile UINT size = count * stride;

	hr = m_Device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(inoutBuffer->m_Buffer.put()));

	if (FAILED(hr))
	{
		hr = m_Device->GetDeviceRemovedReason();
		volatile int a = 0;
		return hr;
	}

	if (data)
	{
		auto commandObject = m_CommandObjectPool->QueryCommandObject(D3D12_COMMAND_LIST_TYPE_DIRECT);
		ID3D12GraphicsCommandList* commandList = commandObject->GetCommandList();

		hr = m_Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&uploadBuffer));

		if (FAILED(hr))
			return hr;

		UINT8* pVertexDataBegin = nullptr;
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.

		ThrowIfFailed(uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, data, size);
		uploadBuffer->Unmap(0, nullptr);

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(inoutBuffer->m_Buffer.get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
		commandList->CopyBufferRegion(inoutBuffer->m_Buffer.get(), 0, uploadBuffer.get(), 0, size);
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(inoutBuffer->m_Buffer.get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

		commandObject->Finish(true);

	}

	return hr;

}

void DX12ResourceManager::UpdateTexturesForWrite(ID3D12Resource* pDestTexResource, ID3D12Resource* pSrcTexResource)
{
	const DWORD MAX_SUB_RESOURCE_NUM = 32;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint[MAX_SUB_RESOURCE_NUM] = {};
	UINT	Rows[MAX_SUB_RESOURCE_NUM] = {};
	UINT64	RowSize[MAX_SUB_RESOURCE_NUM] = {};
	UINT64	TotalBytes = 0;

	D3D12_RESOURCE_DESC Desc = pDestTexResource->GetDesc();
	if (Desc.MipLevels > (UINT)_countof(Footprint))
		__debugbreak();

	m_Device->GetCopyableFootprints(&Desc, 0, Desc.MipLevels, 0, Footprint, Rows, RowSize, &TotalBytes);

	auto commandObject = m_CommandObjectPool->QueryCommandObject(D3D12_COMMAND_LIST_TYPE_DIRECT);
	ID3D12GraphicsCommandList* commandList = commandObject->GetCommandList();

	for (DWORD i = 0; i < Desc.MipLevels; i++)
	{

		D3D12_TEXTURE_COPY_LOCATION	destLocation = {};
		destLocation.PlacedFootprint = Footprint[i];
		destLocation.pResource = pDestTexResource;
		destLocation.SubresourceIndex = i;
		destLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		D3D12_TEXTURE_COPY_LOCATION	srcLocation = {};
		srcLocation.PlacedFootprint = Footprint[i];
		srcLocation.pResource = pSrcTexResource;
		srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

		commandList->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, nullptr);
	}

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pDestTexResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE));

	commandObject->Finish(true);
}
