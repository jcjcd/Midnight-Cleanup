#include "pch.h"
#include "DX11Sampler.h"
#include "NeoDX11Context.h"

DX11Sampler::DX11Sampler(RendererContext* context, bool isReflect, DX11ResourceBinding samplerBinding)
	: DX11Sampler(context, isReflect ? D3D11_TEXTURE_ADDRESS_MIRROR : D3D11_TEXTURE_ADDRESS_WRAP, samplerBinding)
{

}

DX11Sampler::DX11Sampler(RendererContext* context, D3D11_TEXTURE_ADDRESS_MODE addressMode, DX11ResourceBinding samplerBinding)
	: m_Slot(samplerBinding.Register), m_AddressMode(addressMode), m_Name(samplerBinding.Name), m_ShaderType(samplerBinding.Type)
{
	//assert((dynamic_cast<NeoDX11Context*>(context) != nullptr) && "Invalid context type");
	auto dx11Context = static_cast<NeoDX11Context*>(context);

	m_SamplerDesc.BorderColor[0] = 0.0f;
	m_SamplerDesc.BorderColor[1] = 0.0f;
	m_SamplerDesc.BorderColor[2] = 0.0f;
	m_SamplerDesc.BorderColor[3] = 0.0f;

	m_SamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;

	// find type in sampler name
	auto type = SamplerType::None;
	if (m_Name.find("Point") != std::string::npos)
		type = SamplerType::Point;
	else if (m_Name.find("Linear") != std::string::npos)
		type = SamplerType::Linear;
	else if (m_Name.find("Anisotropic") != std::string::npos)
		type = SamplerType::Anisotropic;
	else if (m_Name.find("SSAO") != std::string::npos)
		type = SamplerType::SSAO;
	else if (m_Name.find("Shadow") != std::string::npos)
		type = SamplerType::Shadow;

	if (type == SamplerType::None)
		assert(false && "Invalid sampler type");

	m_SamplerDesc.Filter = [type]()
		{
			switch (type)
			{
				case SamplerType::Point:
					return D3D11_FILTER_MIN_MAG_MIP_POINT;
				case SamplerType::Linear:
					return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
				case SamplerType::Anisotropic:
					return D3D11_FILTER_ANISOTROPIC;
				case SamplerType::Shadow:
					return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
				case SamplerType::None:
				default:
					assert(false && "Invalid sampler type");
					return D3D11_FILTER_MIN_MAG_MIP_POINT;
			}
		}();

	m_SamplerDesc.AddressU = addressMode;
	m_SamplerDesc.AddressV = addressMode;
	m_SamplerDesc.AddressW = addressMode;
	m_SamplerDesc.MaxAnisotropy = D3D11_REQ_MAXANISOTROPY;

	if (type == SamplerType::SSAO)
	{
		m_SamplerDesc.BorderColor[0] = 0.0f;
		m_SamplerDesc.BorderColor[1] = 0.0f;
		m_SamplerDesc.BorderColor[2] = 0.0f;
		m_SamplerDesc.BorderColor[3] = 1e5f;
		m_SamplerDesc.MinLOD = 0;
		m_SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	}
	else if (type == SamplerType::Shadow)
	{
		m_SamplerDesc.BorderColor[0] = 0.0f;
		m_SamplerDesc.BorderColor[1] = 0.0f;
		m_SamplerDesc.BorderColor[2] = 0.0f;
		m_SamplerDesc.BorderColor[3] = 0.0f;
		m_SamplerDesc.MaxAnisotropy = 1;
		m_SamplerDesc.MinLOD = 0;
		m_SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
		m_SamplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	}
	else if(type == SamplerType::Anisotropic)
	{
		m_SamplerDesc.MaxAnisotropy = D3D11_REQ_MAXANISOTROPY;
		m_SamplerDesc.MinLOD = -FLT_MAX;
		m_SamplerDesc.MaxLOD = FLT_MAX;
		m_SamplerDesc.MipLODBias = 0.0f;
		m_SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	}
	else if (type == SamplerType::Point)
	{
		m_SamplerDesc.MinLOD = -FLT_MAX;
		m_SamplerDesc.MaxLOD = FLT_MAX;
		m_SamplerDesc.MipLODBias = 0.0f;
		m_SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	}

	else
	{
		m_SamplerDesc.BorderColor[0] = 0.0f;
		m_SamplerDesc.BorderColor[1] = 0.0f;
		m_SamplerDesc.BorderColor[2] = 0.0f;
		m_SamplerDesc.BorderColor[3] = 0.0f;
		m_SamplerDesc.MinLOD = 0;
		m_SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	}

	HRESULT hr = dx11Context->GetDevice()->CreateSamplerState(&m_SamplerDesc, m_SamplerState.GetAddressOf());
	assert(SUCCEEDED(hr) && "Failed to create sampler state");
}

DX11Sampler::~DX11Sampler()
{
	if (m_SamplerState)
		m_SamplerState.Reset();
}

void DX11Sampler::Bind(RendererContext* context)
{
	/*bool isDX11Context = dynamic_cast<NeoDX11Context*>(context) != nullptr;
	assert(isDX11Context && "Invalid context type");*/
	auto dx11Context = static_cast<NeoDX11Context*>(context);

	switch (m_ShaderType)
	{
		case ShaderType::Vertex:
			dx11Context->GetDeviceContext()->VSSetSamplers(m_Slot, 1, m_SamplerState.GetAddressOf());
			break;
		case ShaderType::Pixel:
			dx11Context->GetDeviceContext()->PSSetSamplers(m_Slot, 1, m_SamplerState.GetAddressOf());
			break;
		case ShaderType::Compute:
			dx11Context->GetDeviceContext()->CSSetSamplers(m_Slot, 1, m_SamplerState.GetAddressOf());
			break;
		case ShaderType::Geometry:
		case ShaderType::StreamOutGeometry:
			dx11Context->GetDeviceContext()->GSSetSamplers(m_Slot, 1, m_SamplerState.GetAddressOf());
			break;
		case ShaderType::Hull:
			dx11Context->GetDeviceContext()->HSSetSamplers(m_Slot, 1, m_SamplerState.GetAddressOf());
			break;
		case ShaderType::Domain:
			dx11Context->GetDeviceContext()->DSSetSamplers(m_Slot, 1, m_SamplerState.GetAddressOf());
			break;
		case ShaderType::Count:
		default:
			assert(false && "Invalid shader type");
			break;
	}

}

