#include "pch.h"
#include "DX11Texture.h"
#include "NeoDX11Context.h"
#include "Renderer.h"

#include <DirectXTex.h>
#include <DirectXTexEXR.h>

DX11Texture::DX11Texture(RendererContext* context, std::string_view path, Type textureType)
{
	m_Path = path;
	m_Type = textureType;
	DirectX::ScratchImage image;
	DirectX::TexMetadata metadata = {};
	std::wstring wpath(m_Path.begin(), m_Path.end());
	//std::wstring extension = wpath.substr(wpath.find_last_of(L".") + 1);
	std::filesystem::path p = path;
	std::wstring extension = p.extension().wstring();
	extension = extension.substr(1);
	std::wstring fileName = wpath.substr(wpath.find_last_of('/') + 1);

	// 확장명이 대문자면 소문자로
	std::ranges::transform(extension, extension.begin(), ::towlower);

	// WOO : 나중에 제대로 된 변환 함수로 수정
	m_Name = winrt::to_string(fileName);

	HRESULT hr = S_OK;

	// WOO : wic flags 필요하면 변경
	if (extension == L"png" || extension == L"jpg" || extension == L"jpeg" || extension == L"tiff" || extension == L"tif" || extension == L"bmp")
		hr = DirectX::LoadFromWICFile(wpath.c_str(), DirectX::WIC_FLAGS_NONE, &metadata, image);

	// WOO : dds flags 필요하면 변경
	else if (extension == L"dds")
		hr = DirectX::LoadFromDDSFile(wpath.c_str(), DirectX::DDS_FLAGS_NONE, &metadata, image);

	else if (extension == L"tga")
		hr = DirectX::LoadFromTGAFile(wpath.c_str(), &metadata, image);

	else if (extension == L"hdr")
		hr = DirectX::LoadFromHDRFile(wpath.c_str(), &metadata, image);

	// unity light map이 exr 파일로 나온다
	else if (extension == L"exr")
		hr = DirectX::LoadFromEXRFile(wpath.c_str(), &metadata, image);

	else
		assert(false && "Invalid texture extension");

	if (metadata.arraySize > 1)
	{
		textureType = Type::Texture2DArray;
		m_Type = textureType;
	}

	if (metadata.IsCubemap())
	{
		textureType = Type::TextureCube;
		m_Type = textureType;
	}

	// 밉맵만들기
	if (metadata.mipLevels == 1)
	{
		DirectX::ScratchImage mipChain;
		hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), metadata, DirectX::TEX_FILTER_DEFAULT, 0, mipChain);
		if (FAILED(hr))
			assert(false && "Failed to generate mipmaps");
		image = std::move(mipChain);
		// 메타데이터 가져오기
		metadata = image.GetMetadata();

		// dds로 저장하기
		// 확장자 dds로 바꾸기
		std::wstring newPath = wpath.substr(0, wpath.find_last_of(L"."));
		newPath += L".dds";
		hr = DirectX::SaveToDDSFile(image.GetImages(), image.GetImageCount(), metadata, DirectX::DDS_FLAGS_NONE, newPath.c_str());

		m_Path = winrt::to_string(newPath);
		fileName = newPath.substr(newPath.find_last_of(L'/') + 1);
		m_Name = winrt::to_string(fileName);
		// 기존 png 삭제하기
		if (wpath != newPath)
		{
			std::filesystem::remove(wpath);
		}
	}


	switch (textureType)
	{
		case Texture::Type::Texture2D:
			createTexture2D(context, image, metadata);
			break;
		case Texture::Type::Texture3D:
			createTexture3D(context, image, metadata);
			break;
		case Texture::Type::TextureCube:
			createTexture2DCube(context, image, metadata);
			break;
		case Texture::Type::Texture2DArray:
			createTexture2DArray(context, image, metadata);
			break;
		case Texture::Type::Unknown:
		default:
			assert(false && "Invalid texture type");
			break;
	}
}

DX11Texture::DX11Texture(RendererContext* context, std::string name, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, Texture::Format format, Texture::Usage usage, float* clearColor, Texture::UAVType uavType, uint32_t arraySize)
	: m_Name(name), m_Usage(usage), m_MipLevels(mipLevels), m_Format(format), m_UAVType(uavType)
{
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	m_Type = type;
	m_Width = width;
	m_Height = height;
	m_ClearValue[0] = clearColor ? clearColor[0] : 0.0f;
	m_ClearValue[1] = clearColor ? clearColor[1] : 0.0f;
	m_ClearValue[2] = clearColor ? clearColor[2] : 0.0f;
	m_ClearValue[3] = clearColor ? clearColor[3] : 1.0f;
	m_ArraySize = arraySize;

	switch (type)
	{
		case Texture::Type::Unknown:
			OutputDebugStringA("Texture type = unknown\n");
			break;
		case Texture::Type::Buffer:
		{
			D3D11_BUFFER_DESC bufferDesc = {};
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			bufferDesc.ByteWidth = width;
			bufferDesc.StructureByteStride = width;
			bufferDesc.CPUAccessFlags = 0;
			bufferDesc.MiscFlags = 0;

			bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(usage, Texture::Usage::RTV))
				bufferDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(usage, Texture::Usage::UAV))
			{
				bufferDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(usage, Texture::Usage::DSV))
				bufferDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateBuffer(&bufferDesc, nullptr, m_TextureBuffer.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = static_cast<DXGI_FORMAT>(format);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(type);
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.ElementOffset = 0;
			srvDesc.Buffer.NumElements = height;
			srvDesc.Buffer.ElementWidth = sizeof(float);

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_TextureBuffer.Get(), &srvDesc, m_SRV.GetAddressOf()));

			if (HasFlag(usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(format);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(type);
				rtvDesc.Buffer.FirstElement = 0;
				rtvDesc.Buffer.NumElements = height;
				rtvDesc.Buffer.ElementWidth = sizeof(float);
				rtvDesc.Buffer.ElementOffset = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateRenderTargetView(m_TextureBuffer.Get(), &rtvDesc, m_RTV.GetAddressOf()));
			}

			if (HasFlag(usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(format);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(type);
				uavDesc.Buffer.FirstElement = 0;
				uavDesc.Buffer.NumElements = height;
				uavDesc.Buffer.Flags |= static_cast<uint32_t>(uavType);

				ThrowIfFailed(dx11Context->GetDevice()->CreateUnorderedAccessView(m_TextureBuffer.Get(), &uavDesc, m_UAV.GetAddressOf()));
			}

			if (HasFlag(usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				dsDesc.Format = static_cast<DXGI_FORMAT>(format);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(type);
				OutputDebugStringA("dx11 dsv does not support buffers\n");
			}
		}
		break;
		case Texture::Type::Texture1D:
		{
			D3D11_TEXTURE1D_DESC tex1dDesc = {};
			tex1dDesc.Width = width;
			tex1dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex1dDesc.ArraySize = arraySize;
			tex1dDesc.Format = static_cast<DXGI_FORMAT>(format);
			tex1dDesc.MipLevels = mipLevels;
			tex1dDesc.CPUAccessFlags = 0;
			tex1dDesc.MiscFlags = 0;

			tex1dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(usage, Texture::Usage::RTV))
				tex1dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(usage, Texture::Usage::UAV))
			{
				tex1dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex1dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex1dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(usage, Texture::Usage::DSV))
				tex1dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture1D(&tex1dDesc, nullptr, m_Texture1D.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = static_cast<DXGI_FORMAT>(format);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(type);
			srvDesc.Texture1D.MostDetailedMip = 0;
			srvDesc.Texture1D.MipLevels = mipLevels;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_Texture1D.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(format);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(type);
				rtvDesc.Texture1D.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateRenderTargetView(m_Texture1D.Get(), &rtvDesc, GetRTVAddress()));
			}

			if (HasFlag(usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(format);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(type);
				uavDesc.Texture1D.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateUnorderedAccessView(m_Texture1D.Get(), &uavDesc, GetUAVAddress()));
			}

			if (HasFlag(usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				dsDesc.Format = static_cast<DXGI_FORMAT>(format);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(static_cast<D3D11_DSV_DIMENSION>(type) - 1);
				dsDesc.Texture1D.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateDepthStencilView(m_Texture1D.Get(), &dsDesc, GetDSVAddress()));
			}
		}
		break;
		case Texture::Type::Texture1DArray:
		{
			D3D11_TEXTURE1D_DESC tex1dDesc = {};
			tex1dDesc.Width = width;
			tex1dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex1dDesc.ArraySize = arraySize;
			tex1dDesc.Format = static_cast<DXGI_FORMAT>(format);
			tex1dDesc.MipLevels = mipLevels;
			tex1dDesc.CPUAccessFlags = 0;
			tex1dDesc.MiscFlags = 0;

			tex1dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(usage, Texture::Usage::RTV))
				tex1dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(usage, Texture::Usage::UAV))
			{
				tex1dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex1dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex1dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(usage, Texture::Usage::DSV))
				tex1dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture1D(&tex1dDesc, nullptr, m_Texture1DArray.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = static_cast<DXGI_FORMAT>(format);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(type);
			srvDesc.Texture1DArray.ArraySize = arraySize;
			srvDesc.Texture1DArray.FirstArraySlice = 0;
			srvDesc.Texture1DArray.MostDetailedMip = 0;
			srvDesc.Texture1DArray.MipLevels = mipLevels;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_Texture1DArray.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(format);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(type);
				rtvDesc.Texture1DArray.ArraySize = arraySize;
				rtvDesc.Texture1DArray.FirstArraySlice = 0;
				rtvDesc.Texture1DArray.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateRenderTargetView(m_Texture1DArray.Get(), &rtvDesc, GetRTVAddress()));
			}

			if (HasFlag(usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(format);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(type);
				uavDesc.Texture1DArray.ArraySize = arraySize;
				uavDesc.Texture1DArray.FirstArraySlice = 0;
				uavDesc.Texture1DArray.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateUnorderedAccessView(m_Texture1DArray.Get(), &uavDesc, GetUAVAddress()));
			}

			if (HasFlag(usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				dsDesc.Format = static_cast<DXGI_FORMAT>(format);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(static_cast<D3D11_DSV_DIMENSION>(type) - 1);
				dsDesc.Texture1DArray.ArraySize = arraySize;
				dsDesc.Texture1DArray.FirstArraySlice = 0;
				dsDesc.Texture1DArray.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateDepthStencilView(m_Texture1DArray.Get(), &dsDesc, GetDSVAddress()));
			}
		}
		break;
		case Texture::Type::Texture2D:
		{
			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.ArraySize = arraySize;
			tex2dDesc.Width = width;
			tex2dDesc.Height = height;
			tex2dDesc.MipLevels = mipLevels;
			tex2dDesc.Format = static_cast<DXGI_FORMAT>(format);
			tex2dDesc.SampleDesc.Count = 1;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex2dDesc.CPUAccessFlags = 0;
			tex2dDesc.MiscFlags = 0;

			tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(usage, Texture::Usage::RTV))
				tex2dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(usage, Texture::Usage::UAV))
			{
				tex2dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(usage, Texture::Usage::DSV))
				tex2dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture2D(&tex2dDesc, nullptr, m_Texture2D.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			if (format == Texture::Format::R24G8_TYPELESS)
				srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			else
				srvDesc.Format = static_cast<DXGI_FORMAT>(format);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(type);
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = mipLevels;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_Texture2D.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(format);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(type);
				rtvDesc.Texture2D.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateRenderTargetView(m_Texture2D.Get(), &rtvDesc, GetRTVAddress()));
			}

			if (HasFlag(usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(format);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(type);
				uavDesc.Texture2D.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateUnorderedAccessView(m_Texture2D.Get(), &uavDesc, GetUAVAddress()));
			}

			if (HasFlag(usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				if (format == Texture::Format::R24G8_TYPELESS)
					dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				else
					dsDesc.Format = static_cast<DXGI_FORMAT>(format);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(static_cast<D3D11_DSV_DIMENSION>(type) - 1);
				dsDesc.Texture2D.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateDepthStencilView(m_Texture2D.Get(), &dsDesc, GetDSVAddress()));
			}
		}
		break;
		case Texture::Type::Texture2DArray:
		{
			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.ArraySize = arraySize;
			tex2dDesc.Width = width;
			tex2dDesc.Height = height;
			tex2dDesc.MipLevels = mipLevels;
			tex2dDesc.Format = static_cast<DXGI_FORMAT>(format);
			tex2dDesc.SampleDesc.Count = 1;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex2dDesc.CPUAccessFlags = 0;
			tex2dDesc.MiscFlags = 0;

			tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(usage, Texture::Usage::RTV))
				tex2dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(usage, Texture::Usage::UAV))
			{
				tex2dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(usage, Texture::Usage::DSV))
				tex2dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture2D(&tex2dDesc, nullptr, m_Texture2DArray.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			if (format == Texture::Format::R24G8_TYPELESS)
				srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			else
				srvDesc.Format = static_cast<DXGI_FORMAT>(format);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(type);
			srvDesc.Texture2DArray.ArraySize = arraySize;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.MipLevels = mipLevels;
			srvDesc.Texture2DArray.MostDetailedMip = 0;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_Texture2DArray.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(format);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(type);
				rtvDesc.Texture2DArray.ArraySize = arraySize;
				rtvDesc.Texture2DArray.FirstArraySlice = 0;
				rtvDesc.Texture2DArray.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateRenderTargetView(m_Texture2DArray.Get(), &rtvDesc, GetRTVAddress()));
			}

			if (HasFlag(usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(format);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(type);
				uavDesc.Texture2DArray.ArraySize = arraySize;
				uavDesc.Texture2DArray.FirstArraySlice = 0;
				uavDesc.Texture2DArray.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateUnorderedAccessView(m_Texture2DArray.Get(), &uavDesc, GetUAVAddress()));
			}

			if (HasFlag(usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				if (format == Texture::Format::R24G8_TYPELESS)
					dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				else
					dsDesc.Format = static_cast<DXGI_FORMAT>(format);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(static_cast<D3D11_DSV_DIMENSION>(type) - 1);
				dsDesc.Texture2DArray.ArraySize = arraySize;
				dsDesc.Texture2DArray.FirstArraySlice = 0;
				dsDesc.Texture2DArray.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateDepthStencilView(m_Texture2DArray.Get(), &dsDesc, m_DSV.GetAddressOf()));
			}
		}
		break;
		case Texture::Type::Texture2DMS:
		{
			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.ArraySize = arraySize;
			tex2dDesc.Width = width;
			tex2dDesc.Height = height;
			tex2dDesc.MipLevels = mipLevels;
			tex2dDesc.Format = static_cast<DXGI_FORMAT>(format);
			tex2dDesc.SampleDesc.Count = 4;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex2dDesc.CPUAccessFlags = 0;
			tex2dDesc.MiscFlags = 0;

			tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(usage, Texture::Usage::RTV))
				tex2dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(usage, Texture::Usage::UAV))
			{
				tex2dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(usage, Texture::Usage::DSV))
				tex2dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture2D(&tex2dDesc, nullptr, m_Texture2DMS.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			if (format == Texture::Format::R24G8_TYPELESS)
				srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			else
				srvDesc.Format = static_cast<DXGI_FORMAT>(format);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(type);
			srvDesc.Texture2DMS.UnusedField_NothingToDefine = 0;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_Texture2DMS.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(format);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(type);
				rtvDesc.Texture2DMS.UnusedField_NothingToDefine = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateRenderTargetView(m_Texture2DMS.Get(), &rtvDesc, GetRTVAddress()));
			}

			if (HasFlag(usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(format);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(type);
				uavDesc.Texture2D.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateUnorderedAccessView(m_Texture2DMS.Get(), &uavDesc, GetUAVAddress()));
			}

			if (HasFlag(usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				if (format == Texture::Format::R24G8_TYPELESS)
					dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				else
					dsDesc.Format = static_cast<DXGI_FORMAT>(format);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(static_cast<D3D11_DSV_DIMENSION>(type) - 1);
				dsDesc.Texture2DMS.UnusedField_NothingToDefine = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateDepthStencilView(m_Texture2DMS.Get(), &dsDesc, GetDSVAddress()));
			}
		}
		break;
		case Texture::Type::Texture2DMSArray:
		{
			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.ArraySize = arraySize;
			tex2dDesc.Width = width;
			tex2dDesc.Height = height;
			tex2dDesc.MipLevels = mipLevels;
			tex2dDesc.Format = static_cast<DXGI_FORMAT>(format);
			tex2dDesc.SampleDesc.Count = 4;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex2dDesc.CPUAccessFlags = 0;
			tex2dDesc.MiscFlags = 0;

			tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(usage, Texture::Usage::RTV))
				tex2dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(usage, Texture::Usage::UAV))
			{
				tex2dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(usage, Texture::Usage::DSV))
				tex2dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture2D(&tex2dDesc, nullptr, m_Texture2DMSArray.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			if (format == Texture::Format::R24G8_TYPELESS)
				srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			else
				srvDesc.Format = static_cast<DXGI_FORMAT>(format);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(type);
			srvDesc.Texture2DMSArray.ArraySize = arraySize;
			srvDesc.Texture2DMSArray.FirstArraySlice = 0;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_Texture2DMSArray.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(format);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(type);
				rtvDesc.Texture2DMSArray.ArraySize = arraySize;
				rtvDesc.Texture2DMSArray.FirstArraySlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateRenderTargetView(m_Texture2DMSArray.Get(), &rtvDesc, GetRTVAddress()));
			}

			if (HasFlag(usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(format);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(type);
				uavDesc.Texture2DArray.ArraySize = arraySize;
				uavDesc.Texture2DArray.FirstArraySlice = 0;
				uavDesc.Texture2DArray.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateUnorderedAccessView(m_Texture2DMSArray.Get(), &uavDesc, GetUAVAddress()));
			}

			if (HasFlag(usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				if (format == Texture::Format::R24G8_TYPELESS)
					dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				else
					dsDesc.Format = static_cast<DXGI_FORMAT>(format);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(static_cast<D3D11_DSV_DIMENSION>(type) - 1);
				dsDesc.Texture2DMSArray.ArraySize = arraySize;
				dsDesc.Texture2DMSArray.FirstArraySlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateDepthStencilView(m_Texture2DMSArray.Get(), &dsDesc, GetDSVAddress()));
			}
		}
		break;
		case Texture::Type::Texture3D:
		{
			D3D11_TEXTURE3D_DESC tex3dDesc = {};
			tex3dDesc.Width = width;
			tex3dDesc.Height = height;
			tex3dDesc.Depth = 1;
			tex3dDesc.MipLevels = mipLevels;
			tex3dDesc.Format = static_cast<DXGI_FORMAT>(format);
			tex3dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex3dDesc.CPUAccessFlags = 0;
			tex3dDesc.MiscFlags = 0;

			tex3dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(usage, Texture::Usage::RTV))
				tex3dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(usage, Texture::Usage::UAV))
			{
				tex3dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex3dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex3dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(usage, Texture::Usage::DSV))
				tex3dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture3D(&tex3dDesc, nullptr, m_Texture3D.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = static_cast<DXGI_FORMAT>(format);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(type);
			srvDesc.Texture3D.MostDetailedMip = 0;
			srvDesc.Texture3D.MipLevels = mipLevels;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_Texture3D.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(format);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(type);
				rtvDesc.Texture3D.MipSlice = 0;
				rtvDesc.Texture3D.FirstWSlice = 0;
				rtvDesc.Texture3D.WSize = 1;

				ThrowIfFailed(dx11Context->GetDevice()->CreateRenderTargetView(m_Texture3D.Get(), &rtvDesc, GetRTVAddress()));
			}

			if (HasFlag(usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(format);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(type);
				uavDesc.Texture3D.MipSlice = 0;
				uavDesc.Texture3D.FirstWSlice = 0;
				uavDesc.Texture3D.WSize = 1;

				ThrowIfFailed(dx11Context->GetDevice()->CreateUnorderedAccessView(m_Texture3D.Get(), &uavDesc, GetUAVAddress()));
			}

			if (HasFlag(usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				dsDesc.Format = static_cast<DXGI_FORMAT>(format);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(type);
				OutputDebugStringA("dsv does not support Texture3D");
			}
		}
		break;
		case Texture::Type::TextureCube:
		{
			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.ArraySize = 6;
			tex2dDesc.Width = width;
			tex2dDesc.Height = height;
			tex2dDesc.MipLevels = mipLevels;
			tex2dDesc.Format = static_cast<DXGI_FORMAT>(format);
			tex2dDesc.SampleDesc.Count = 1;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex2dDesc.CPUAccessFlags = 0;
			tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

			tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(usage, Texture::Usage::RTV))
				tex2dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(usage, Texture::Usage::UAV))
			{
				tex2dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(usage, Texture::Usage::DSV))
				tex2dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture2D(&tex2dDesc, nullptr, m_TextureCube.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			if (format == Texture::Format::R24G8_TYPELESS)
				srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			else
				srvDesc.Format = static_cast<DXGI_FORMAT>(format);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(type);
			srvDesc.TextureCube.MipLevels = mipLevels;
			srvDesc.TextureCube.MostDetailedMip = 0;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_TextureCube.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(format);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(type);
				rtvDesc.Texture2DArray.ArraySize = 6;
				rtvDesc.Texture2DArray.FirstArraySlice = 0;
				rtvDesc.Texture2DArray.MipSlice = 0;

			}

			if (HasFlag(usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(format);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(type);
				uavDesc.Texture2DArray.ArraySize = 6;
				uavDesc.Texture2DArray.FirstArraySlice = 0;
				uavDesc.Texture2DArray.MipSlice = 0;

			}

			if (HasFlag(usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				if (format == Texture::Format::R24G8_TYPELESS)
					dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				else
					dsDesc.Format = static_cast<DXGI_FORMAT>(format);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(type);
				OutputDebugStringA("Texture3D does not support texture cube");

			}
		}
		break;
		case Texture::Type::TextureCubeArray:
		{
			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.ArraySize = 6 * arraySize;
			tex2dDesc.Width = width;
			tex2dDesc.Height = height;
			tex2dDesc.MipLevels = mipLevels;
			tex2dDesc.Format = static_cast<DXGI_FORMAT>(format);
			tex2dDesc.SampleDesc.Count = 1;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex2dDesc.CPUAccessFlags = 0;
			tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

			tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(usage, Texture::Usage::RTV))
				tex2dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(usage, Texture::Usage::UAV))
			{
				tex2dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(uavType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(usage, Texture::Usage::DSV))
				tex2dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture2D(&tex2dDesc, nullptr, m_TextureCubeArray.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = static_cast<DXGI_FORMAT>(format);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(type);
			srvDesc.TextureCubeArray.First2DArrayFace = 0;
			srvDesc.TextureCubeArray.MipLevels = mipLevels;
			srvDesc.TextureCubeArray.MostDetailedMip = 0;
			srvDesc.TextureCubeArray.NumCubes = 1;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_TextureCubeArray.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(format);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(type);
				rtvDesc.Texture2DArray.ArraySize = arraySize;
				rtvDesc.Texture2DArray.FirstArraySlice = 0;
				rtvDesc.Texture2DArray.MipSlice = 0;
			}

			if (HasFlag(usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(format);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(type);
				uavDesc.Texture2DArray.ArraySize = arraySize;
				uavDesc.Texture2DArray.FirstArraySlice = 0;
				uavDesc.Texture2DArray.MipSlice = 0;
			}

			if (HasFlag(usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				dsDesc.Format = static_cast<DXGI_FORMAT>(format);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(type);
				OutputDebugStringA("Texture3D does not support texture cube array");
			}
		}
		break;
		default:
			OutputDebugStringA("invalid texture type");
			break;
	}
}

DX11Texture::DX11Texture(RendererContext* context, std::string name, uint32_t width,
	uint32_t height, Texture::Format format, bool isDynamic)
{
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	m_Name = name;
	m_Type = Texture::Type::Texture2D;
	m_Width = width;
	m_Height = height;
	m_Format = format;

	D3D11_TEXTURE2D_DESC tex2dDesc = {};
	tex2dDesc.Width = width;
	tex2dDesc.Height = height;
	tex2dDesc.MipLevels = 1;
	tex2dDesc.ArraySize = 1;
	tex2dDesc.Format = static_cast<DXGI_FORMAT>(format);
	tex2dDesc.SampleDesc.Count = 1;
	tex2dDesc.SampleDesc.Quality = 0;
	tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
	tex2dDesc.CPUAccessFlags = 0;
	tex2dDesc.MiscFlags = 0;

	tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	if (isDynamic)
	{
		tex2dDesc.Usage = D3D11_USAGE_DYNAMIC;
		tex2dDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else
		tex2dDesc.Usage = D3D11_USAGE_DEFAULT;


	ThrowIfFailed(dx11Context->GetDevice()->CreateTexture2D(&tex2dDesc, nullptr, m_Texture2D.GetAddressOf()));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = static_cast<DXGI_FORMAT>(format);
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = tex2dDesc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;

	ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_Texture2D.Get(), &srvDesc, m_SRV.GetAddressOf()));
}

DX11Texture::DX11Texture(RendererContext* context, const TextureDesc& desc)
{
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	m_Name = desc.name;
	m_Usage = desc.usage;
	m_MipLevels = desc.mipLevels;
	m_Format = desc.format;
	m_UAVType = desc.uavType;
	m_Type = desc.type;
	m_Width = desc.width;
	m_Height = desc.height;
	m_ArraySize = desc.arraySize;
	m_ClearValue[0] = desc.clearColor ? desc.clearColor[0] : 0.0f;
	m_ClearValue[1] = desc.clearColor ? desc.clearColor[1] : 0.0f;
	m_ClearValue[2] = desc.clearColor ? desc.clearColor[2] : 0.0f;
	m_ClearValue[3] = desc.clearColor ? desc.clearColor[3] : 1.0f;

	switch (desc.type)
	{
		case Texture::Type::Unknown:
			OutputDebugStringA("Texture type = unknown\n");
			break;
		case Texture::Type::Buffer:
		{
			D3D11_BUFFER_DESC bufferDesc = {};
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			bufferDesc.ByteWidth = desc.width;
			bufferDesc.StructureByteStride = desc.width;
			bufferDesc.CPUAccessFlags = 0;
			bufferDesc.MiscFlags = 0;

			bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(desc.usage, Texture::Usage::RTV))
				bufferDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(desc.usage, Texture::Usage::UAV))
			{
				bufferDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(desc.usage, Texture::Usage::DSV))
				bufferDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateBuffer(&bufferDesc, nullptr, m_TextureBuffer.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = static_cast<DXGI_FORMAT>(desc.srvFormat);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(desc.type);
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.ElementOffset = 0;
			srvDesc.Buffer.NumElements = desc.height;
			srvDesc.Buffer.ElementWidth = sizeof(float);

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_TextureBuffer.Get(), &srvDesc, m_SRV.GetAddressOf()));

			if (HasFlag(desc.usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(desc.rtvFormat);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(desc.type);
				rtvDesc.Buffer.FirstElement = 0;
				rtvDesc.Buffer.NumElements = desc.height;
				rtvDesc.Buffer.ElementWidth = sizeof(float);
				rtvDesc.Buffer.ElementOffset = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateRenderTargetView(m_TextureBuffer.Get(), &rtvDesc, m_RTV.GetAddressOf()));
			}

			if (HasFlag(desc.usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(desc.uavFormat);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(desc.type);
				uavDesc.Buffer.FirstElement = 0;
				uavDesc.Buffer.NumElements = desc.height;
				uavDesc.Buffer.Flags |= static_cast<uint32_t>(desc.uavType);

				ThrowIfFailed(dx11Context->GetDevice()->CreateUnorderedAccessView(m_TextureBuffer.Get(), &uavDesc, m_UAV.GetAddressOf()));
			}

			if (HasFlag(desc.usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				dsDesc.Format = static_cast<DXGI_FORMAT>(desc.dsvFormat);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(desc.type);
				OutputDebugStringA("dx11 dsv does not support buffers\n");
			}
		}
		break;
		case Texture::Type::Texture1D:
		{
			D3D11_TEXTURE1D_DESC tex1dDesc = {};
			tex1dDesc.Width = desc.width;
			tex1dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex1dDesc.ArraySize = desc.arraySize;
			tex1dDesc.Format = static_cast<DXGI_FORMAT>(desc.format);
			tex1dDesc.MipLevels = desc.mipLevels;
			tex1dDesc.CPUAccessFlags = 0;
			tex1dDesc.MiscFlags = 0;

			tex1dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(desc.usage, Texture::Usage::RTV))
				tex1dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(desc.usage, Texture::Usage::UAV))
			{
				tex1dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex1dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex1dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(desc.usage, Texture::Usage::DSV))
				tex1dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture1D(&tex1dDesc, nullptr, m_Texture1D.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = static_cast<DXGI_FORMAT>(desc.srvFormat);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(desc.type);
			srvDesc.Texture1D.MostDetailedMip = 0;
			srvDesc.Texture1D.MipLevels = desc.mipLevels;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_Texture1D.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(desc.usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(desc.rtvFormat);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(desc.type);
				rtvDesc.Texture1D.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateRenderTargetView(m_Texture1D.Get(), &rtvDesc, GetRTVAddress()));
			}

			if (HasFlag(desc.usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(desc.uavFormat);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(desc.type);
				uavDesc.Texture1D.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateUnorderedAccessView(m_Texture1D.Get(), &uavDesc, GetUAVAddress()));
			}

			if (HasFlag(desc.usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				dsDesc.Format = static_cast<DXGI_FORMAT>(desc.dsvFormat);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(static_cast<D3D11_DSV_DIMENSION>(desc.type) - 1);
				dsDesc.Texture1D.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateDepthStencilView(m_Texture1D.Get(), &dsDesc, GetDSVAddress()));
			}
		}
		break;
		case Texture::Type::Texture1DArray:
		{
			D3D11_TEXTURE1D_DESC tex1dDesc = {};
			tex1dDesc.Width = desc.width;
			tex1dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex1dDesc.ArraySize = desc.arraySize;
			tex1dDesc.Format = static_cast<DXGI_FORMAT>(desc.format);
			tex1dDesc.MipLevels = desc.mipLevels;
			tex1dDesc.CPUAccessFlags = 0;
			tex1dDesc.MiscFlags = 0;

			tex1dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(desc.usage, Texture::Usage::RTV))
				tex1dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(desc.usage, Texture::Usage::UAV))
			{
				tex1dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex1dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex1dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(desc.usage, Texture::Usage::DSV))
				tex1dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture1D(&tex1dDesc, nullptr, m_Texture1DArray.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = static_cast<DXGI_FORMAT>(desc.srvFormat);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(desc.type);
			srvDesc.Texture1DArray.ArraySize = desc.arraySize;
			srvDesc.Texture1DArray.FirstArraySlice = 0;
			srvDesc.Texture1DArray.MostDetailedMip = 0;
			srvDesc.Texture1DArray.MipLevels = desc.mipLevels;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_Texture1DArray.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(desc.usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(desc.rtvFormat);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(desc.type);
				rtvDesc.Texture1DArray.ArraySize = desc.arraySize;
				rtvDesc.Texture1DArray.FirstArraySlice = 0;
				rtvDesc.Texture1DArray.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateRenderTargetView(m_Texture1DArray.Get(), &rtvDesc, GetRTVAddress()));
			}

			if (HasFlag(desc.usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(desc.uavFormat);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(desc.type);
				uavDesc.Texture1DArray.ArraySize = desc.arraySize;
				uavDesc.Texture1DArray.FirstArraySlice = 0;
				uavDesc.Texture1DArray.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateUnorderedAccessView(m_Texture1DArray.Get(), &uavDesc, GetUAVAddress()));
			}

			if (HasFlag(desc.usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				dsDesc.Format = static_cast<DXGI_FORMAT>(desc.dsvFormat);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(static_cast<D3D11_DSV_DIMENSION>(desc.type) - 1);
				dsDesc.Texture1DArray.ArraySize = desc.arraySize;
				dsDesc.Texture1DArray.FirstArraySlice = 0;
				dsDesc.Texture1DArray.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateDepthStencilView(m_Texture1DArray.Get(), &dsDesc, GetDSVAddress()));
			}
		}
		break;
		case Texture::Type::Texture2D:
		{
			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.ArraySize = desc.arraySize;
			tex2dDesc.Width = desc.width;
			tex2dDesc.Height = desc.height;
			tex2dDesc.MipLevels = desc.mipLevels;
			tex2dDesc.Format = static_cast<DXGI_FORMAT>(desc.format);
			tex2dDesc.SampleDesc.Count = 1;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex2dDesc.CPUAccessFlags = 0;
			tex2dDesc.MiscFlags = 0;

			tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(desc.usage, Texture::Usage::RTV))
				tex2dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(desc.usage, Texture::Usage::UAV))
			{
				tex2dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(desc.usage, Texture::Usage::DSV))
				tex2dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture2D(&tex2dDesc, nullptr, m_Texture2D.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = static_cast<DXGI_FORMAT>(desc.srvFormat);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(desc.type);
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = desc.mipLevels;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_Texture2D.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(desc.usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(desc.rtvFormat);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(desc.type);
				rtvDesc.Texture2D.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateRenderTargetView(m_Texture2D.Get(), &rtvDesc, GetRTVAddress()));
			}

			if (HasFlag(desc.usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(desc.uavFormat);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(desc.type);
				uavDesc.Texture2D.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateUnorderedAccessView(m_Texture2D.Get(), &uavDesc, GetUAVAddress()));
			}

			if (HasFlag(desc.usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				dsDesc.Format = static_cast<DXGI_FORMAT>(desc.dsvFormat);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(static_cast<D3D11_DSV_DIMENSION>(desc.type) - 1);
				dsDesc.Texture2D.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateDepthStencilView(m_Texture2D.Get(), &dsDesc, GetDSVAddress()));
			}
		}
		break;
		case Texture::Type::Texture2DArray:
		{
			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.ArraySize = desc.arraySize;
			tex2dDesc.Width = desc.width;
			tex2dDesc.Height = desc.height;
			tex2dDesc.MipLevels = desc.mipLevels;
			tex2dDesc.Format = static_cast<DXGI_FORMAT>(desc.format);
			tex2dDesc.SampleDesc.Count = 1;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex2dDesc.CPUAccessFlags = 0;
			tex2dDesc.MiscFlags = 0;

			tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(desc.usage, Texture::Usage::RTV))
				tex2dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(desc.usage, Texture::Usage::UAV))
			{
				tex2dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(desc.usage, Texture::Usage::DSV))
				tex2dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture2D(&tex2dDesc, nullptr, m_Texture2DArray.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = static_cast<DXGI_FORMAT>(desc.srvFormat);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(desc.type);
			srvDesc.Texture2DArray.ArraySize = desc.arraySize;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.MipLevels = desc.mipLevels;
			srvDesc.Texture2DArray.MostDetailedMip = 0;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_Texture2DArray.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(desc.usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(desc.rtvFormat);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(desc.type);
				rtvDesc.Texture2DArray.ArraySize = desc.arraySize;
				rtvDesc.Texture2DArray.FirstArraySlice = 0;
				rtvDesc.Texture2DArray.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateRenderTargetView(m_Texture2DArray.Get(), &rtvDesc, GetRTVAddress()));
			}

			if (HasFlag(desc.usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(desc.uavFormat);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(desc.type);
				uavDesc.Texture2DArray.ArraySize = desc.arraySize;
				uavDesc.Texture2DArray.FirstArraySlice = 0;
				uavDesc.Texture2DArray.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateUnorderedAccessView(m_Texture2DArray.Get(), &uavDesc, GetUAVAddress()));
			}

			if (HasFlag(desc.usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				dsDesc.Format = static_cast<DXGI_FORMAT>(desc.dsvFormat);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(static_cast<D3D11_DSV_DIMENSION>(desc.type) - 1);
				dsDesc.Texture2DArray.ArraySize = desc.arraySize;
				dsDesc.Texture2DArray.FirstArraySlice = 0;
				dsDesc.Texture2DArray.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateDepthStencilView(m_Texture2DArray.Get(), &dsDesc, m_DSV.GetAddressOf()));

				// set debug name 예제
				/*static const char c_szName[] = "test dsv";
				m_Texture2DArray->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(c_szName) - 1, c_szName);
				m_DSV->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(c_szName) - 1, c_szName);*/
			}
		}
		break;
		case Texture::Type::Texture2DMS:
		{
			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.ArraySize = desc.arraySize;
			tex2dDesc.Width = desc.width;
			tex2dDesc.Height = desc.height;
			tex2dDesc.MipLevels = desc.mipLevels;
			tex2dDesc.Format = static_cast<DXGI_FORMAT>(desc.format);
			tex2dDesc.SampleDesc.Count = 4;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex2dDesc.CPUAccessFlags = 0;
			tex2dDesc.MiscFlags = 0;

			tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(desc.usage, Texture::Usage::RTV))
				tex2dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(desc.usage, Texture::Usage::UAV))
			{
				tex2dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(desc.usage, Texture::Usage::DSV))
				tex2dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture2D(&tex2dDesc, nullptr, m_Texture2DMS.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = static_cast<DXGI_FORMAT>(desc.srvFormat);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(desc.type);
			srvDesc.Texture2DMS.UnusedField_NothingToDefine = 0;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_Texture2DMS.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(desc.usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(desc.rtvFormat);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(desc.type);
				rtvDesc.Texture2DMS.UnusedField_NothingToDefine = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateRenderTargetView(m_Texture2DMS.Get(), &rtvDesc, GetRTVAddress()));
			}

			if (HasFlag(desc.usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(desc.uavFormat);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(desc.type);
				uavDesc.Texture2D.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateUnorderedAccessView(m_Texture2DMS.Get(), &uavDesc, GetUAVAddress()));
			}

			if (HasFlag(desc.usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				dsDesc.Format = static_cast<DXGI_FORMAT>(desc.dsvFormat);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(static_cast<D3D11_DSV_DIMENSION>(desc.type) - 1);
				dsDesc.Texture2DMS.UnusedField_NothingToDefine = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateDepthStencilView(m_Texture2DMS.Get(), &dsDesc, GetDSVAddress()));
			}
		}
		break;
		case Texture::Type::Texture2DMSArray:
		{
			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.ArraySize = desc.arraySize;
			tex2dDesc.Width = desc.width;
			tex2dDesc.Height = desc.height;
			tex2dDesc.MipLevels = desc.mipLevels;
			tex2dDesc.Format = static_cast<DXGI_FORMAT>(desc.format);
			tex2dDesc.SampleDesc.Count = 4;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex2dDesc.CPUAccessFlags = 0;
			tex2dDesc.MiscFlags = 0;

			tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(desc.usage, Texture::Usage::RTV))
				tex2dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(desc.usage, Texture::Usage::UAV))
			{
				tex2dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(desc.usage, Texture::Usage::DSV))
				tex2dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture2D(&tex2dDesc, nullptr, m_Texture2DMSArray.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = static_cast<DXGI_FORMAT>(desc.srvFormat);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(desc.type);
			srvDesc.Texture2DMSArray.ArraySize = desc.arraySize;
			srvDesc.Texture2DMSArray.FirstArraySlice = 0;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_Texture2DMSArray.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(desc.usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(desc.rtvFormat);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(desc.type);
				rtvDesc.Texture2DMSArray.ArraySize = desc.arraySize;
				rtvDesc.Texture2DMSArray.FirstArraySlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateRenderTargetView(m_Texture2DMSArray.Get(), &rtvDesc, GetRTVAddress()));
			}

			if (HasFlag(desc.usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(desc.uavFormat);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(desc.type);
				uavDesc.Texture2DArray.ArraySize = desc.arraySize;
				uavDesc.Texture2DArray.FirstArraySlice = 0;
				uavDesc.Texture2DArray.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateUnorderedAccessView(m_Texture2DMSArray.Get(), &uavDesc, GetUAVAddress()));
			}

			if (HasFlag(desc.usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				dsDesc.Format = static_cast<DXGI_FORMAT>(desc.dsvFormat);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(static_cast<D3D11_DSV_DIMENSION>(desc.type) - 1);
				dsDesc.Texture2DMSArray.ArraySize = desc.arraySize;
				dsDesc.Texture2DMSArray.FirstArraySlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateDepthStencilView(m_Texture2DMSArray.Get(), &dsDesc, GetDSVAddress()));
			}
		}
		break;
		case Texture::Type::Texture3D:
		{
			D3D11_TEXTURE3D_DESC tex3dDesc = {};
			tex3dDesc.Width = desc.width;
			tex3dDesc.Height = desc.height;
			tex3dDesc.Depth = 1;
			tex3dDesc.MipLevels = desc.mipLevels;
			tex3dDesc.Format = static_cast<DXGI_FORMAT>(desc.format);
			tex3dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex3dDesc.CPUAccessFlags = 0;
			tex3dDesc.MiscFlags = 0;

			tex3dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(desc.usage, Texture::Usage::RTV))
				tex3dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(desc.usage, Texture::Usage::UAV))
			{
				tex3dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex3dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex3dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(desc.usage, Texture::Usage::DSV))
				tex3dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture3D(&tex3dDesc, nullptr, m_Texture3D.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = static_cast<DXGI_FORMAT>(desc.srvFormat);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(desc.type);
			srvDesc.Texture3D.MostDetailedMip = 0;
			srvDesc.Texture3D.MipLevels = desc.mipLevels;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_Texture3D.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(desc.usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(desc.rtvFormat);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(desc.type);
				rtvDesc.Texture3D.MipSlice = 0;
				rtvDesc.Texture3D.FirstWSlice = 0;
				rtvDesc.Texture3D.WSize = 1;

				ThrowIfFailed(dx11Context->GetDevice()->CreateRenderTargetView(m_Texture3D.Get(), &rtvDesc, GetRTVAddress()));
			}

			if (HasFlag(desc.usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(desc.uavFormat);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(desc.type);
				uavDesc.Texture3D.MipSlice = 0;
				uavDesc.Texture3D.FirstWSlice = 0;
				uavDesc.Texture3D.WSize = 1;

				ThrowIfFailed(dx11Context->GetDevice()->CreateUnorderedAccessView(m_Texture3D.Get(), &uavDesc, GetUAVAddress()));
			}

			if (HasFlag(desc.usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				dsDesc.Format = static_cast<DXGI_FORMAT>(desc.dsvFormat);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(desc.type);
				OutputDebugStringA("dsv does not support Texture3D");
			}
		}
		break;
		case Texture::Type::TextureCube:
		{
			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.ArraySize = 6;
			tex2dDesc.Width = desc.width;
			tex2dDesc.Height = desc.height;
			tex2dDesc.MipLevels = desc.mipLevels;
			tex2dDesc.Format = static_cast<DXGI_FORMAT>(desc.format);
			tex2dDesc.SampleDesc.Count = 1;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex2dDesc.CPUAccessFlags = 0;
			tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

			tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(desc.usage, Texture::Usage::RTV))
				tex2dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(desc.usage, Texture::Usage::UAV))
			{
				tex2dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(desc.usage, Texture::Usage::DSV))
				tex2dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture2D(&tex2dDesc, nullptr, m_TextureCube.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = static_cast<DXGI_FORMAT>(desc.srvFormat);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(desc.type);
			srvDesc.TextureCube.MipLevels = desc.mipLevels;
			srvDesc.TextureCube.MostDetailedMip = 0;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_TextureCube.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(desc.usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(desc.rtvFormat);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(desc.type);
				rtvDesc.Texture2DArray.ArraySize = 6;
				rtvDesc.Texture2DArray.FirstArraySlice = 0;
				rtvDesc.Texture2DArray.MipSlice = 0;

			}

			if (HasFlag(desc.usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(desc.uavFormat);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(desc.type);
				uavDesc.Texture2DArray.ArraySize = 6;
				uavDesc.Texture2DArray.FirstArraySlice = 0;
				uavDesc.Texture2DArray.MipSlice = 0;

			}

			if (HasFlag(desc.usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				dsDesc.Format = static_cast<DXGI_FORMAT>(desc.dsvFormat);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(desc.type);
				OutputDebugStringA("Texture3D does not support texture cube");

			}
		}
		break;
		case Texture::Type::TextureCubeArray:
		{
			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.ArraySize = 6 * desc.arraySize;
			tex2dDesc.Width = desc.width;
			tex2dDesc.Height = desc.height;
			tex2dDesc.MipLevels = desc.mipLevels;
			tex2dDesc.Format = static_cast<DXGI_FORMAT>(desc.format);
			tex2dDesc.SampleDesc.Count = 1;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex2dDesc.CPUAccessFlags = 0;
			tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

			tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(desc.usage, Texture::Usage::RTV))
				tex2dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(desc.usage, Texture::Usage::UAV))
			{
				tex2dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(desc.uavType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(desc.usage, Texture::Usage::DSV))
				tex2dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture2D(&tex2dDesc, nullptr, m_TextureCubeArray.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = static_cast<DXGI_FORMAT>(desc.srvFormat);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(desc.type);
			srvDesc.TextureCubeArray.First2DArrayFace = 0;
			srvDesc.TextureCubeArray.MipLevels = desc.mipLevels;
			srvDesc.TextureCubeArray.MostDetailedMip = 0;
			srvDesc.TextureCubeArray.NumCubes = 1;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_TextureCubeArray.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(desc.usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(desc.rtvFormat);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(desc.type);
				rtvDesc.Texture2DArray.ArraySize = desc.arraySize;
				rtvDesc.Texture2DArray.FirstArraySlice = 0;
				rtvDesc.Texture2DArray.MipSlice = 0;
			}

			if (HasFlag(desc.usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(desc.uavFormat);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(desc.type);
				uavDesc.Texture2DArray.ArraySize = desc.arraySize;
				uavDesc.Texture2DArray.FirstArraySlice = 0;
				uavDesc.Texture2DArray.MipSlice = 0;
			}

			if (HasFlag(desc.usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				dsDesc.Format = static_cast<DXGI_FORMAT>(desc.dsvFormat);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(desc.type);
				OutputDebugStringA("Texture3D does not support texture cube array");
			}
		}
		break;
		default:
			OutputDebugStringA("invalid texture type");
			break;
	}
}

DX11Texture::DX11Texture(RendererContext* context, std::string_view path, DirectX::TexMetadata metadata, DirectX::ScratchImage& image)
{
	m_Path = path;
	std::wstring wpath(m_Path.begin(), m_Path.end());
	std::wstring fileName = wpath.substr(wpath.find_last_of('/') + 1);

	m_Name = winrt::to_string(fileName);

	switch (metadata.dimension)
	{
		case DirectX::TEX_DIMENSION_TEXTURE1D:
			break;
		case DirectX::TEX_DIMENSION_TEXTURE2D:
			if (metadata.arraySize == 1)
			{
				m_Type = Texture::Type::Texture2D;
				createTexture2D(context, image, metadata);
			}
			else
			{
				if (metadata.IsCubemap())
				{
					m_Type = Texture::Type::TextureCube;
					createTexture2DCube(context, image, metadata);
				}
				else
				{
					m_Type = Texture::Type::Texture2DArray;
					createTexture2DArray(context, image, metadata);
				}
			}
			break;
		case DirectX::TEX_DIMENSION_TEXTURE3D:
		{
			m_Type = Texture::Type::Texture3D;
			createTexture3D(context, image, metadata);
		}
		break;
		default:
			break;
	}
}

DX11Texture::~DX11Texture()
{
	m_Buffer.Reset();
	m_SRV.Reset();
	m_UAV.Reset();
	m_RTV.Reset();
	m_DSV.Reset();
	m_Texture1D.Reset();
	m_Texture1DArray.Reset();
	m_Texture2D.Reset();
	m_Texture2DArray.Reset();
	m_Texture2DMS.Reset();
	m_Texture2DMSArray.Reset();
	m_Texture3D.Reset();
	m_TextureCube.Reset();
	m_TextureCubeArray.Reset();
	m_TextureBuffer.Reset();
}

void DX11Texture::Bind(RendererContext* context, const DX11ResourceBinding& textureBinding)
{
	//bool isContextDX11 = dynamic_cast<NeoDX11Context*>(context) != nullptr;
	//assert(isContextDX11 && "Invalid context type");
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	m_Slot = textureBinding.Register;
	m_ShaderType = textureBinding.Type;

	switch (m_ShaderType)
	{
		case ShaderType::Vertex:
			dx11Context->GetDeviceContext()->VSSetShaderResources(m_Slot, 1, m_SRV.GetAddressOf());
			break;
		case ShaderType::Pixel:
			dx11Context->GetDeviceContext()->PSSetShaderResources(m_Slot, 1, m_SRV.GetAddressOf());
			break;
		case ShaderType::Compute:
			dx11Context->GetDeviceContext()->CSSetShaderResources(m_Slot, 1, m_SRV.GetAddressOf());
			break;
		case ShaderType::Geometry:
		case ShaderType::StreamOutGeometry:
			dx11Context->GetDeviceContext()->GSSetShaderResources(m_Slot, 1, m_SRV.GetAddressOf());
			break;
		case ShaderType::Hull:
			dx11Context->GetDeviceContext()->HSSetShaderResources(m_Slot, 1, m_SRV.GetAddressOf());
			break;
		case ShaderType::Domain:
			dx11Context->GetDeviceContext()->DSSetShaderResources(m_Slot, 1, m_SRV.GetAddressOf());
			break;
		case ShaderType::Count:
		default:
			assert(false && "Invalid shader type");
			break;
	}
}

void DX11Texture::Unbind(RendererContext* context)
{
	/*bool isContextDX11 = dynamic_cast<NeoDX11Context*>(context) != nullptr;
	assert(isContextDX11 && "Invalid context type");*/
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	ID3D11ShaderResourceView* nullSRV[] = { nullptr };

	switch (m_ShaderType)
	{
		case ShaderType::Vertex:
			dx11Context->GetDeviceContext()->VSSetShaderResources(m_Slot, 1, nullSRV);
			break;
		case ShaderType::Pixel:
			dx11Context->GetDeviceContext()->PSSetShaderResources(m_Slot, 1, nullSRV);
			break;
		case ShaderType::Compute:
			dx11Context->GetDeviceContext()->CSSetShaderResources(m_Slot, 1, nullSRV);
			break;
		case ShaderType::Geometry:
		case ShaderType::StreamOutGeometry:
			dx11Context->GetDeviceContext()->GSSetShaderResources(m_Slot, 1, nullSRV);
			break;
		case ShaderType::Hull:
			dx11Context->GetDeviceContext()->HSSetShaderResources(m_Slot, 1, nullSRV);
			break;
		case ShaderType::Domain:
			dx11Context->GetDeviceContext()->DSSetShaderResources(m_Slot, 1, nullSRV);
			break;
		case ShaderType::Count:
		default:
			assert(false && "Invalid shader type");
			break;
	}
}

void DX11Texture::BindUAV(RendererContext* context, const DX11ResourceBinding& uavBinding, uint32_t numRenderTarget /*= 1*/, ID3D11RenderTargetView* rtvPtr[] /*= nullptr*/, ID3D11DepthStencilView* dsv /*= nullptr*/)
{
	bool isContextDX11 = dynamic_cast<NeoDX11Context*>(context) != nullptr;
	assert(isContextDX11 && "Invalid context type");
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	m_Slot = uavBinding.Register;
	m_ShaderType = uavBinding.Type;

	bool needInitialCount = (uavBinding.ResourceType == D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER ||
		uavBinding.ResourceType == D3D_SIT_UAV_CONSUME_STRUCTURED ||
		uavBinding.ResourceType == D3D_SIT_UAV_APPEND_STRUCTURED);

	switch (m_ShaderType)
	{
		case ShaderType::Pixel:
		{
			if (needInitialCount)
			{
				uint32_t initialCount[] = { static_cast<uint32_t>(m_InitialCount) };
				dx11Context->GetDeviceContext()->OMSetRenderTargetsAndUnorderedAccessViews(numRenderTarget, rtvPtr, dsv, m_Slot, 1, m_UAV.GetAddressOf(), initialCount);
			}
			else
				dx11Context->GetDeviceContext()->OMSetRenderTargetsAndUnorderedAccessViews(numRenderTarget, rtvPtr, dsv, m_Slot, 1, m_UAV.GetAddressOf(), nullptr);
		}
		break;
		case ShaderType::Compute:
		{
			if (needInitialCount)
			{
				uint32_t initialCount[] = { static_cast<uint32_t>(m_InitialCount) };
				dx11Context->GetDeviceContext()->CSSetUnorderedAccessViews(m_Slot, 1, m_UAV.GetAddressOf(), initialCount);
			}
			else
				dx11Context->GetDeviceContext()->CSSetUnorderedAccessViews(m_Slot, 1, m_UAV.GetAddressOf(), nullptr);
		}
		break;
		default:
			assert(false && "Invalid shader type");
			break;
	}

}

void DX11Texture::UnbindUAV(RendererContext* context)
{
	/*bool isContextDX11 = dynamic_cast<NeoDX11Context*>(context) != nullptr;
	assert(isContextDX11 && "Invalid context type");*/
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	ID3D11UnorderedAccessView* nullUAV[] = { nullptr };
	ID3D11RenderTargetView* nullRTV[] = { nullptr };

	switch (m_ShaderType)
	{
		case ShaderType::Pixel:
			dx11Context->GetDeviceContext()->OMSetRenderTargetsAndUnorderedAccessViews(ARRAYSIZE(nullRTV), nullRTV, nullptr, m_Slot, 1, nullUAV, nullptr);
			break;
		case ShaderType::Compute:
			dx11Context->GetDeviceContext()->CSSetUnorderedAccessViews(m_Slot, 1, nullUAV, nullptr);
			break;
	}
}

void DX11Texture::Resize(RendererContext* context, uint32_t width, uint32_t height)
{
	m_Texture2D.Reset();
	m_Texture2DArray.Reset();
	m_Texture2DMS.Reset();
	m_Texture2DMSArray.Reset();
	m_Texture3D.Reset();
	m_TextureCube.Reset();
	m_TextureCubeArray.Reset();
	m_TextureBuffer.Reset();
	m_SRV.Reset();
	m_UAV.Reset();
	m_RTV.Reset();
	m_DSV.Reset();
	m_Texture1D.Reset();
	m_Texture1DArray.Reset();
	m_Buffer.Reset();

	m_Width = width;
	m_Height = height;

	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	switch (m_Type)
	{
		case Texture::Type::Unknown:
			OutputDebugStringA("Texture type = unknown\n");
			break;
		case Texture::Type::Buffer:
		{
			D3D11_BUFFER_DESC bufferDesc = {};
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			bufferDesc.ByteWidth = width;
			bufferDesc.StructureByteStride = width;
			bufferDesc.CPUAccessFlags = 0;
			bufferDesc.MiscFlags = 0;

			bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(m_Usage, Texture::Usage::RTV))
				bufferDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(m_Usage, Texture::Usage::UAV))
			{
				bufferDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(m_Usage, Texture::Usage::DSV))
				bufferDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateBuffer(&bufferDesc, nullptr, m_TextureBuffer.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(m_Type);
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.ElementOffset = 0;
			srvDesc.Buffer.NumElements = height;
			srvDesc.Buffer.ElementWidth = sizeof(float);

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_TextureBuffer.Get(), &srvDesc, m_SRV.GetAddressOf()));

			if (HasFlag(m_Usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(m_Type);
				rtvDesc.Buffer.FirstElement = 0;
				rtvDesc.Buffer.NumElements = height;
				rtvDesc.Buffer.ElementWidth = sizeof(float);
				rtvDesc.Buffer.ElementOffset = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateRenderTargetView(m_TextureBuffer.Get(), &rtvDesc, m_RTV.GetAddressOf()));
			}

			if (HasFlag(m_Usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(m_Type);
				uavDesc.Buffer.FirstElement = 0;
				uavDesc.Buffer.NumElements = height;
				uavDesc.Buffer.Flags |= static_cast<uint32_t>(m_UAVType);

				ThrowIfFailed(dx11Context->GetDevice()->CreateUnorderedAccessView(m_TextureBuffer.Get(), &uavDesc, m_UAV.GetAddressOf()));
			}

			if (HasFlag(m_Usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				dsDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(m_Type);
				OutputDebugStringA("dx11 dsv does not support buffers\n");
			}
		}
		break;
		case Texture::Type::Texture1D:
		{
			D3D11_TEXTURE1D_DESC tex1dDesc = {};
			tex1dDesc.Width = width;
			tex1dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex1dDesc.ArraySize = m_ArraySize;
			tex1dDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
			tex1dDesc.MipLevels = m_MipLevels;
			tex1dDesc.CPUAccessFlags = 0;
			tex1dDesc.MiscFlags = 0;

			tex1dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(m_Usage, Texture::Usage::RTV))
				tex1dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(m_Usage, Texture::Usage::UAV))
			{
				tex1dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex1dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex1dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(m_Usage, Texture::Usage::DSV))
				tex1dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture1D(&tex1dDesc, nullptr, m_Texture1D.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(m_Type);
			srvDesc.Texture1D.MostDetailedMip = 0;
			srvDesc.Texture1D.MipLevels = m_MipLevels;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_Texture1D.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(m_Usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(m_Type);
				rtvDesc.Texture1D.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateRenderTargetView(m_Texture1D.Get(), &rtvDesc, GetRTVAddress()));
			}

			if (HasFlag(m_Usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(m_Type);
				uavDesc.Texture1D.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateUnorderedAccessView(m_Texture1D.Get(), &uavDesc, GetUAVAddress()));
			}

			if (HasFlag(m_Usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				dsDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(static_cast<D3D11_DSV_DIMENSION>(m_Type) - 1);
				dsDesc.Texture1D.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateDepthStencilView(m_Texture1D.Get(), &dsDesc, GetDSVAddress()));
			}
		}
		break;
		case Texture::Type::Texture1DArray:
		{
			D3D11_TEXTURE1D_DESC tex1dDesc = {};
			tex1dDesc.Width = width;
			tex1dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex1dDesc.ArraySize = m_ArraySize;
			tex1dDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
			tex1dDesc.MipLevels = m_MipLevels;
			tex1dDesc.CPUAccessFlags = 0;
			tex1dDesc.MiscFlags = 0;

			tex1dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(m_Usage, Texture::Usage::RTV))
				tex1dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(m_Usage, Texture::Usage::UAV))
			{
				tex1dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex1dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex1dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(m_Usage, Texture::Usage::DSV))
				tex1dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture1D(&tex1dDesc, nullptr, m_Texture1DArray.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(m_Type);
			srvDesc.Texture1DArray.ArraySize = m_ArraySize;
			srvDesc.Texture1DArray.FirstArraySlice = 0;
			srvDesc.Texture1DArray.MostDetailedMip = 0;
			srvDesc.Texture1DArray.MipLevels = m_MipLevels;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_Texture1DArray.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(m_Usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(m_Type);
				rtvDesc.Texture1DArray.ArraySize = m_ArraySize;
				rtvDesc.Texture1DArray.FirstArraySlice = 0;
				rtvDesc.Texture1DArray.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateRenderTargetView(m_Texture1DArray.Get(), &rtvDesc, GetRTVAddress()));
			}

			if (HasFlag(m_Usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(m_Type);
				uavDesc.Texture1DArray.ArraySize = m_ArraySize;
				uavDesc.Texture1DArray.FirstArraySlice = 0;
				uavDesc.Texture1DArray.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateUnorderedAccessView(m_Texture1DArray.Get(), &uavDesc, GetUAVAddress()));
			}

			if (HasFlag(m_Usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				dsDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(static_cast<D3D11_DSV_DIMENSION>(m_Type) - 1);
				dsDesc.Texture1DArray.ArraySize = m_ArraySize;
				dsDesc.Texture1DArray.FirstArraySlice = 0;
				dsDesc.Texture1DArray.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateDepthStencilView(m_Texture1DArray.Get(), &dsDesc, GetDSVAddress()));
			}
		}
		break;
		case Texture::Type::Texture2D:
		{
			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.ArraySize = m_ArraySize;
			tex2dDesc.Width = width;
			tex2dDesc.Height = height;
			tex2dDesc.MipLevels = m_MipLevels;
			tex2dDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
			tex2dDesc.SampleDesc.Count = 1;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex2dDesc.CPUAccessFlags = 0;
			tex2dDesc.MiscFlags = 0;

			tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(m_Usage, Texture::Usage::RTV))
				tex2dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(m_Usage, Texture::Usage::UAV))
			{
				tex2dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(m_Usage, Texture::Usage::DSV))
				tex2dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture2D(&tex2dDesc, nullptr, m_Texture2D.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			if (m_Format == Texture::Format::R24G8_TYPELESS)
				srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			else
				srvDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(m_Type);
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = m_MipLevels;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_Texture2D.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(m_Usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(m_Type);
				rtvDesc.Texture2D.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateRenderTargetView(m_Texture2D.Get(), &rtvDesc, GetRTVAddress()));
			}

			if (HasFlag(m_Usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(m_Type);
				uavDesc.Texture2D.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateUnorderedAccessView(m_Texture2D.Get(), &uavDesc, GetUAVAddress()));
			}

			if (HasFlag(m_Usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				if (m_Format == Texture::Format::R24G8_TYPELESS)
					dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				else
					dsDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(static_cast<D3D11_DSV_DIMENSION>(m_Type) - 1);
				dsDesc.Texture2D.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateDepthStencilView(m_Texture2D.Get(), &dsDesc, GetDSVAddress()));
			}
		}
		break;
		case Texture::Type::Texture2DArray:
		{
			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.ArraySize = m_ArraySize;
			tex2dDesc.Width = width;
			tex2dDesc.Height = height;
			tex2dDesc.MipLevels = m_MipLevels;
			tex2dDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
			tex2dDesc.SampleDesc.Count = 1;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex2dDesc.CPUAccessFlags = 0;
			tex2dDesc.MiscFlags = 0;

			tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(m_Usage, Texture::Usage::RTV))
				tex2dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(m_Usage, Texture::Usage::UAV))
			{
				tex2dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(m_Usage, Texture::Usage::DSV))
				tex2dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture2D(&tex2dDesc, nullptr, m_Texture2DArray.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			if (m_Format == Texture::Format::R24G8_TYPELESS)
				srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			else
				srvDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(m_Type);
			srvDesc.Texture2DArray.ArraySize = m_ArraySize;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.MipLevels = m_MipLevels;
			srvDesc.Texture2DArray.MostDetailedMip = 0;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_Texture2DArray.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(m_Usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(m_Type);
				rtvDesc.Texture2DArray.ArraySize = m_ArraySize;
				rtvDesc.Texture2DArray.FirstArraySlice = 0;
				rtvDesc.Texture2DArray.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateRenderTargetView(m_Texture2DArray.Get(), &rtvDesc, GetRTVAddress()));
			}

			if (HasFlag(m_Usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(m_Type);
				uavDesc.Texture2DArray.ArraySize = m_ArraySize;
				uavDesc.Texture2DArray.FirstArraySlice = 0;
				uavDesc.Texture2DArray.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateUnorderedAccessView(m_Texture2DArray.Get(), &uavDesc, GetUAVAddress()));
			}

			if (HasFlag(m_Usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				if (m_Format == Texture::Format::R24G8_TYPELESS)
					dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				else
					dsDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(static_cast<D3D11_DSV_DIMENSION>(m_Type) - 1);
				dsDesc.Texture2DArray.ArraySize = m_ArraySize;
				dsDesc.Texture2DArray.FirstArraySlice = 0;
				dsDesc.Texture2DArray.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateDepthStencilView(m_Texture2DArray.Get(), &dsDesc, m_DSV.GetAddressOf()));
			}
		}
		break;
		case Texture::Type::Texture2DMS:
		{
			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.ArraySize = m_ArraySize;
			tex2dDesc.Width = width;
			tex2dDesc.Height = height;
			tex2dDesc.MipLevels = m_MipLevels;
			tex2dDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
			tex2dDesc.SampleDesc.Count = 4;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex2dDesc.CPUAccessFlags = 0;
			tex2dDesc.MiscFlags = 0;

			tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(m_Usage, Texture::Usage::RTV))
				tex2dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(m_Usage, Texture::Usage::UAV))
			{
				tex2dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(m_Usage, Texture::Usage::DSV))
				tex2dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture2D(&tex2dDesc, nullptr, m_Texture2DMS.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			if (m_Format == Texture::Format::R24G8_TYPELESS)
				srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			else
				srvDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(m_Type);
			srvDesc.Texture2DMS.UnusedField_NothingToDefine = 0;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_Texture2DMS.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(m_Usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(m_Type);
				rtvDesc.Texture2DMS.UnusedField_NothingToDefine = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateRenderTargetView(m_Texture2DMS.Get(), &rtvDesc, GetRTVAddress()));
			}

			if (HasFlag(m_Usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(m_Type);
				uavDesc.Texture2D.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateUnorderedAccessView(m_Texture2DMS.Get(), &uavDesc, GetUAVAddress()));
			}

			if (HasFlag(m_Usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				if (m_Format == Texture::Format::R24G8_TYPELESS)
					dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				else
					dsDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(static_cast<D3D11_DSV_DIMENSION>(m_Type) - 1);
				dsDesc.Texture2DMS.UnusedField_NothingToDefine = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateDepthStencilView(m_Texture2DMS.Get(), &dsDesc, GetDSVAddress()));
			}
		}
		break;
		case Texture::Type::Texture2DMSArray:
		{
			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.ArraySize = m_ArraySize;
			tex2dDesc.Width = width;
			tex2dDesc.Height = height;
			tex2dDesc.MipLevels = m_MipLevels;
			tex2dDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
			tex2dDesc.SampleDesc.Count = 4;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex2dDesc.CPUAccessFlags = 0;
			tex2dDesc.MiscFlags = 0;

			tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(m_Usage, Texture::Usage::RTV))
				tex2dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(m_Usage, Texture::Usage::UAV))
			{
				tex2dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(m_Usage, Texture::Usage::DSV))
				tex2dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture2D(&tex2dDesc, nullptr, m_Texture2DMSArray.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			if (m_Format == Texture::Format::R24G8_TYPELESS)
				srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			else
				srvDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(m_Type);
			srvDesc.Texture2DMSArray.ArraySize = m_ArraySize;
			srvDesc.Texture2DMSArray.FirstArraySlice = 0;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_Texture2DMSArray.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(m_Usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(m_Type);
				rtvDesc.Texture2DMSArray.ArraySize = m_ArraySize;
				rtvDesc.Texture2DMSArray.FirstArraySlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateRenderTargetView(m_Texture2DMSArray.Get(), &rtvDesc, GetRTVAddress()));
			}

			if (HasFlag(m_Usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(m_Type);
				uavDesc.Texture2DArray.ArraySize = m_ArraySize;
				uavDesc.Texture2DArray.FirstArraySlice = 0;
				uavDesc.Texture2DArray.MipSlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateUnorderedAccessView(m_Texture2DMSArray.Get(), &uavDesc, GetUAVAddress()));
			}

			if (HasFlag(m_Usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				if (m_Format == Texture::Format::R24G8_TYPELESS)
					dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				else
					dsDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(static_cast<D3D11_DSV_DIMENSION>(m_Type) - 1);
				dsDesc.Texture2DMSArray.ArraySize = m_ArraySize;
				dsDesc.Texture2DMSArray.FirstArraySlice = 0;

				ThrowIfFailed(dx11Context->GetDevice()->CreateDepthStencilView(m_Texture2DMSArray.Get(), &dsDesc, GetDSVAddress()));
			}
		}
		break;
		case Texture::Type::Texture3D:
		{
			D3D11_TEXTURE3D_DESC tex3dDesc = {};
			tex3dDesc.Width = width;
			tex3dDesc.Height = height;
			tex3dDesc.Depth = 1;
			tex3dDesc.MipLevels = m_MipLevels;
			tex3dDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
			tex3dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex3dDesc.CPUAccessFlags = 0;
			tex3dDesc.MiscFlags = 0;

			tex3dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(m_Usage, Texture::Usage::RTV))
				tex3dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(m_Usage, Texture::Usage::UAV))
			{
				tex3dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex3dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex3dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(m_Usage, Texture::Usage::DSV))
				tex3dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture3D(&tex3dDesc, nullptr, m_Texture3D.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(m_Type);
			srvDesc.Texture3D.MostDetailedMip = 0;
			srvDesc.Texture3D.MipLevels = m_MipLevels;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_Texture3D.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(m_Usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(m_Type);
				rtvDesc.Texture3D.MipSlice = 0;
				rtvDesc.Texture3D.FirstWSlice = 0;
				rtvDesc.Texture3D.WSize = 1;

				ThrowIfFailed(dx11Context->GetDevice()->CreateRenderTargetView(m_Texture3D.Get(), &rtvDesc, GetRTVAddress()));
			}

			if (HasFlag(m_Usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(m_Type);
				uavDesc.Texture3D.MipSlice = 0;
				uavDesc.Texture3D.FirstWSlice = 0;
				uavDesc.Texture3D.WSize = 1;

				ThrowIfFailed(dx11Context->GetDevice()->CreateUnorderedAccessView(m_Texture3D.Get(), &uavDesc, GetUAVAddress()));
			}

			if (HasFlag(m_Usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				dsDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(m_Type);
				OutputDebugStringA("dsv does not support Texture3D");
			}
		}
		break;
		case Texture::Type::TextureCube:
		{
			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.ArraySize = 6;
			tex2dDesc.Width = width;
			tex2dDesc.Height = height;
			tex2dDesc.MipLevels = m_MipLevels;
			tex2dDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
			tex2dDesc.SampleDesc.Count = 1;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex2dDesc.CPUAccessFlags = 0;
			tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

			tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(m_Usage, Texture::Usage::RTV))
				tex2dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(m_Usage, Texture::Usage::UAV))
			{
				tex2dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(m_Usage, Texture::Usage::DSV))
				tex2dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture2D(&tex2dDesc, nullptr, m_TextureCube.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			if (m_Format == Texture::Format::R24G8_TYPELESS)
				srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			else
				srvDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(m_Type);
			srvDesc.TextureCube.MipLevels = m_MipLevels;
			srvDesc.TextureCube.MostDetailedMip = 0;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_TextureCube.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(m_Usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(m_Type);
				rtvDesc.Texture2DArray.ArraySize = 6;
				rtvDesc.Texture2DArray.FirstArraySlice = 0;
				rtvDesc.Texture2DArray.MipSlice = 0;

			}

			if (HasFlag(m_Usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(m_Type);
				uavDesc.Texture2DArray.ArraySize = 6;
				uavDesc.Texture2DArray.FirstArraySlice = 0;
				uavDesc.Texture2DArray.MipSlice = 0;

			}

			if (HasFlag(m_Usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				if (m_Format == Texture::Format::R24G8_TYPELESS)
					dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				else
					dsDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(m_Type);
				OutputDebugStringA("Texture3D does not support texture cube");

			}
		}
		break;
		case Texture::Type::TextureCubeArray:
		{
			D3D11_TEXTURE2D_DESC tex2dDesc = {};
			tex2dDesc.ArraySize = 6 * m_ArraySize;
			tex2dDesc.Width = width;
			tex2dDesc.Height = height;
			tex2dDesc.MipLevels = m_MipLevels;
			tex2dDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
			tex2dDesc.SampleDesc.Count = 1;
			tex2dDesc.SampleDesc.Quality = 0;
			tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
			tex2dDesc.CPUAccessFlags = 0;
			tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

			tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			if (HasFlag(m_Usage, Texture::Usage::RTV))
				tex2dDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			if (HasFlag(m_Usage, Texture::Usage::UAV))
			{
				tex2dDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
				if (static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::APPEND) || static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::COUNTER))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				else if (static_cast<uint32_t>(m_UAVType) & static_cast<uint32_t>(Texture::UAVType::RAW))
					tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			}
			if (HasFlag(m_Usage, Texture::Usage::DSV))
				tex2dDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

			ThrowIfFailed(dx11Context->GetDevice()->CreateTexture2D(&tex2dDesc, nullptr, m_TextureCubeArray.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
			srvDesc.ViewDimension = static_cast<D3D11_SRV_DIMENSION>(m_Type);
			srvDesc.TextureCubeArray.First2DArrayFace = 0;
			srvDesc.TextureCubeArray.MipLevels = m_MipLevels;
			srvDesc.TextureCubeArray.MostDetailedMip = 0;
			srvDesc.TextureCubeArray.NumCubes = 1;

			ThrowIfFailed(dx11Context->GetDevice()->CreateShaderResourceView(m_TextureCubeArray.Get(), &srvDesc, GetSRVAddress()));

			if (HasFlag(m_Usage, Texture::Usage::RTV))
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				rtvDesc.ViewDimension = static_cast<D3D11_RTV_DIMENSION>(m_Type);
				rtvDesc.Texture2DArray.ArraySize = m_ArraySize * 6;
				rtvDesc.Texture2DArray.FirstArraySlice = 0;
				rtvDesc.Texture2DArray.MipSlice = 0;
			}

			if (HasFlag(m_Usage, Texture::Usage::UAV))
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				uavDesc.ViewDimension = static_cast<D3D11_UAV_DIMENSION>(m_Type);
				uavDesc.Texture2DArray.ArraySize = m_ArraySize * 6;
				uavDesc.Texture2DArray.FirstArraySlice = 0;
				uavDesc.Texture2DArray.MipSlice = 0;
			}

			if (HasFlag(m_Usage, Texture::Usage::DSV))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
				dsDesc.Format = static_cast<DXGI_FORMAT>(m_Format);
				dsDesc.ViewDimension = static_cast<D3D11_DSV_DIMENSION>(m_Type);
				OutputDebugStringA("Texture3D does not support texture cube array");
			}
		}
		break;
		default:
			OutputDebugStringA("invalid texture type");
			break;
	}
}

bool DX11Texture::UpdateFromIYUV(RendererContext* context, const uint8_t* data, size_t dataSize)
{
	assert(data);

	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	// Create the texture
	auto ctx = dx11Context->GetDeviceContext();

	D3D11_MAPPED_SUBRESOURCE ms;
	HRESULT hr = ctx->Map(m_Texture2D.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	if (FAILED(hr))
		return false;

	uint32_t bytes_per_texel = 1;

	const uint8_t* src = data;
	uint8_t* dst = (uint8_t*)ms.pData;

	// Copy the Y lines
	uint32_t y_lines = m_Height / 2;
	uint32_t bytes_per_row = m_Width * bytes_per_texel;
	for (uint32_t y = 0; y < y_lines; ++y) {
		memcpy(dst, src, bytes_per_row);
		src += bytes_per_row;
		dst += ms.RowPitch;
	}

	// Now the U and V lines
	uint32_t uv_lines = m_Height / 2;
	uint32_t uv_bytes_per_row = bytes_per_row / 2;
	for (uint32_t y = 0; y < uv_lines; ++y) {
		memcpy(dst, src, uv_bytes_per_row);
		src += uv_bytes_per_row;
		dst += ms.RowPitch;
	}

	ctx->Unmap(m_Texture2D.Get(), 0);
	return true;
}

void* DX11Texture::GetShaderResourceView()
{
	return reinterpret_cast<void*>(m_SRV.Get());
}

std::shared_ptr<DX11Texture> DX11Texture::Create(Renderer* renderer, const std::string& path, Type type)
{
	return static_pointer_cast<DX11Texture>(renderer->CreateTexture(path.c_str()));
}

std::shared_ptr<DX11Texture> DX11Texture::Create(Renderer* renderer, const std::string& path, Type type, bool isUI)
{
	return static_pointer_cast<DX11Texture>(renderer->CreateTexture(path.c_str(), isUI));
}

void DX11Texture::createTexture2D(RendererContext* context, const DirectX::ScratchImage& image, const DirectX::TexMetadata& metadata)
{
	/*bool isContextDX11 = dynamic_cast<NeoDX11Context*>(context) != nullptr;
	assert(isContextDX11 && "Invalid context type");*/
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);




	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = static_cast<uint32_t>(metadata.width);
	desc.Height = static_cast<uint32_t>(metadata.height);
	desc.MipLevels = static_cast<uint32_t>(metadata.mipLevels);
	desc.ArraySize = static_cast<uint32_t>(metadata.arraySize);
	desc.Format = metadata.format;
	desc.SampleDesc.Count = 1;	// WOO : multisampling 사용하면 고려
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;	// WOO : flag가 더 필요한게 있는지 검토
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;	// WOO : flag가 더 필요한게 있는지 검토

	std::vector<D3D11_SUBRESOURCE_DATA> subresourceData(metadata.mipLevels);
	const uint8_t* pData = image.GetPixels();

	uint32_t width = static_cast<uint32_t>(metadata.width);
	uint32_t height = static_cast<uint32_t>(metadata.height);

	for (uint32_t i = 0; i < metadata.mipLevels; ++i)
	{
		subresourceData[i].pSysMem = pData;
		subresourceData[i].SysMemPitch = static_cast<uint32_t>(width * DirectX::BitsPerPixel(metadata.format) / 8);
		subresourceData[i].SysMemSlicePitch = subresourceData[i].SysMemPitch * height;

		// 다음 Mipmap 레벨로 이동
		pData += subresourceData[i].SysMemSlicePitch;
		width = (std::max)(1u, width / 2);
		height = (std::max)(1u, height / 2);
	}


	HRESULT hr = dx11Context->GetDevice()->CreateTexture2D(&desc, subresourceData.data(), m_Texture2D.GetAddressOf());

	assert(SUCCEEDED(hr) && "Failed to create texture");

	// srv 생성
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = metadata.format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = desc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;

	hr = dx11Context->GetDevice()->CreateShaderResourceView(m_Texture2D.Get(), &srvDesc, m_SRV.GetAddressOf());
	assert(SUCCEEDED(hr) && "Failed to create srv");
}

void DX11Texture::createTexture2DCube(RendererContext* context, const DirectX::ScratchImage& image, const DirectX::TexMetadata& metadata)
{
	/*bool isContextDX11 = dynamic_cast<NeoDX11Context*>(context) != nullptr;
	assert(isContextDX11 && "Invalid context type");*/
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = static_cast<uint32_t>(metadata.width);
	desc.Height = static_cast<uint32_t>(metadata.height);
	desc.MipLevels = static_cast<uint32_t>(metadata.mipLevels);
	desc.ArraySize = static_cast<uint32_t>(metadata.arraySize);
	if (desc.ArraySize != 6)
		assert(false && "Invalid cubemap texture");

	desc.Format = metadata.format;
	desc.SampleDesc.Count = 1;	// WOO : multisampling 사용하면 고려
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;	// WOO : flag가 더 필요한게 있는지 검토

	//D3D11_SUBRESOURCE_DATA initData[6] = {};
	//for (int i = 0; i < 6; i++)
	//{
	//	initData[i].pSysMem = image.GetImage(i, 0, 0)->pixels;
	//	// WOO : pitch 계산하는게 맞는지 확인, format에 따라서 8의 배수가 아니면 문제가 생길 수 있음
	//	initData[i].SysMemPitch = static_cast<uint32_t>(metadata.width * DirectX::BitsPerPixel(metadata.format) / 8);
	//	initData[i].SysMemSlicePitch = 0;
	//}

	std::vector<D3D11_SUBRESOURCE_DATA> subresources;
	subresources.reserve(metadata.mipLevels * metadata.arraySize);

	for (size_t i = 0; i < metadata.arraySize; i++)
	{
		for (size_t j = 0; j < metadata.mipLevels; j++)
		{
			auto img = image.GetImage(j, i, 0);

			D3D11_SUBRESOURCE_DATA data = {};
			data.pSysMem = img->pixels;
			data.SysMemPitch = static_cast<uint32_t>(img->rowPitch);
			data.SysMemSlicePitch = static_cast<uint32_t>(img->slicePitch);
			subresources.emplace_back(data);
		}
	}

	HRESULT hr = dx11Context->GetDevice()->CreateTexture2D(&desc, subresources.data(), m_TextureCube.GetAddressOf());
	assert(SUCCEEDED(hr) && "Failed to create texture");

	// srv 생성
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = metadata.format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MipLevels = desc.MipLevels;
	srvDesc.TextureCube.MostDetailedMip = 0;

	hr = dx11Context->GetDevice()->CreateShaderResourceView(m_TextureCube.Get(), &srvDesc, m_SRV.GetAddressOf());
	assert(SUCCEEDED(hr) && "Failed to create srv");
}

void DX11Texture::createTexture3D(RendererContext* context, const DirectX::ScratchImage& image, const DirectX::TexMetadata& metadata)
{
	/*bool isContextDX11 = dynamic_cast<NeoDX11Context*>(context) != nullptr;
	assert(isContextDX11 && "Invalid context type");*/
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	D3D11_TEXTURE3D_DESC desc = {};
	desc.Width = static_cast<uint32_t>(metadata.width);
	desc.Height = static_cast<uint32_t>(metadata.height);
	desc.Depth = static_cast<uint32_t>(metadata.depth);
	desc.MipLevels = static_cast<uint32_t>(metadata.mipLevels);
	desc.Format = metadata.format;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;	// WOO : flag가 더 필요한게 있는지 검토

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = image.GetPixels();
	// WOO : pitch 계산하는게 맞는지 확인, format에 따라서 8의 배수가 아니면 문제가 생길 수 있음
	initData.SysMemPitch = static_cast<uint32_t>(metadata.width * DirectX::BitsPerPixel(metadata.format) / 8);
	initData.SysMemSlicePitch = static_cast<uint32_t>(metadata.width * metadata.height * DirectX::BitsPerPixel(metadata.format) / 8);

	HRESULT hr = dx11Context->GetDevice()->CreateTexture3D(&desc, &initData, m_Texture3D.GetAddressOf());
	assert(SUCCEEDED(hr) && "Failed to create texture");

	// srv 생성
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = metadata.format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	srvDesc.Texture3D.MipLevels = desc.MipLevels;
	srvDesc.Texture3D.MostDetailedMip = 0;

	hr = dx11Context->GetDevice()->CreateShaderResourceView(m_Texture3D.Get(), &srvDesc, m_SRV.GetAddressOf());
	assert(SUCCEEDED(hr) && "Failed to create srv");
}

void DX11Texture::createTexture2DArray(RendererContext* context, const DirectX::ScratchImage& image, const DirectX::TexMetadata& metadata)
{
	/*bool isContextDX11 = dynamic_cast<NeoDX11Context*>(context) != nullptr;
	assert(isContextDX11 && "Invalid context type");*/
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = static_cast<uint32_t>(metadata.width);
	desc.Height = static_cast<uint32_t>(metadata.height);
	desc.MipLevels = static_cast<uint32_t>(metadata.mipLevels);
	desc.ArraySize = static_cast<uint32_t>(metadata.arraySize);
	desc.Format = metadata.format;
	desc.SampleDesc.Count = 1;	// WOO : multisampling 사용하면 고려
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;	// WOO : flag가 더 필요한게 있는지 검토

	std::vector<D3D11_SUBRESOURCE_DATA> initData(metadata.mipLevels * metadata.arraySize);
	// WOO : pitch 계산하는게 맞는지 확인, format에 따라서 8의 배수가 아니면 문제가 생길 수 있음
	size_t pixelSize = DirectX::BitsPerPixel(metadata.format) / 8;

	for (uint32_t arrayIndex = 0; arrayIndex < metadata.arraySize; arrayIndex++)
	{
		for (uint32_t mipLevel = 0; mipLevel < metadata.mipLevels; mipLevel++)
		{
			uint32_t subresourceIndex = arrayIndex * static_cast<uint32_t>(metadata.mipLevels) + mipLevel;
			const DirectX::Image* img = image.GetImage(mipLevel, arrayIndex, 0);
			initData[subresourceIndex].pSysMem = img->pixels;
			initData[subresourceIndex].SysMemPitch = static_cast<uint32_t>(img->rowPitch);
			initData[subresourceIndex].SysMemSlicePitch = static_cast<uint32_t>(img->slicePitch);
		}
	}

	HRESULT hr = dx11Context->GetDevice()->CreateTexture2D(&desc, initData.data(), m_Texture2DArray.GetAddressOf());
	assert(SUCCEEDED(hr) && "Failed to create texture");

	// srv 생성
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = metadata.format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Texture2DArray.MipLevels = desc.MipLevels;
	srvDesc.Texture2DArray.MostDetailedMip = 0;
	srvDesc.Texture2DArray.FirstArraySlice = 0;
	srvDesc.Texture2DArray.ArraySize = desc.ArraySize;

	hr = dx11Context->GetDevice()->CreateShaderResourceView(m_Texture2DArray.Get(), &srvDesc, m_SRV.GetAddressOf());
	assert(SUCCEEDED(hr) && "Failed to create srv");
}
